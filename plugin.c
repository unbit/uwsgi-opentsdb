#include <uwsgi.h>

/*

this is a stats pusher plugin for the opentsdb server:

--stats-push opentsdb:address tags

example:

--stats-push opentsdb:127.0.0.1:4242 myinstance

it exports values exposed by the metric subsystem

*/

extern struct uwsgi_server uwsgi;

// configuration of a opentsdb node
struct opentsdb_node {
	char *addr;
	char *tags;
	size_t tags_len;
};

static int opentsdb_add_metric(struct uwsgi_buffer *ub, struct uwsgi_stats_pusher_instance *uspi, time_t now, char *metric, size_t metric_len, int64_t value) {
	struct opentsdb_node *on = (struct opentsdb_node *) uspi->data;
	// reset the buffer
	if (uwsgi_buffer_append(ub, "put ", 4)) return -1;	
	if (uwsgi_buffer_append(ub, metric, metric_len)) return -1;
	if (uwsgi_buffer_append(ub, " ", 1)) return -1;
        if (uwsgi_buffer_num64(ub, now)) return -1;
	if (uwsgi_buffer_append(ub, " ", 1)) return -1;
        if (uwsgi_buffer_num64(ub, value)) return -1;
	if (uwsgi_buffer_append(ub, " ", 1)) return -1;
	if (uwsgi_buffer_append(ub, on->tags, on->tags_len)) return -1;
	if (uwsgi_buffer_append(ub, "\n", 1)) return -1;
        return 0;
}


static void stats_pusher_opentsdb(struct uwsgi_stats_pusher_instance *uspi, time_t now, char *json, size_t json_len) {

	if (!uspi->configured) {
		struct opentsdb_node *on = uwsgi_calloc(sizeof(struct opentsdb_node));
		char *space = strchr(uspi->arg, ' ');
		if (!space) {
			uwsgi_log("invalid opentsdb syntax: %s\n", uspi->arg);
			free(on);
			return;
		}
		on->addr = uwsgi_concat2n(uspi->arg, space - uspi->arg, "" , 0);
		on->tags = space+1;
		on->tags_len = strlen(space+1);
		uspi->data = on;
		uspi->configured = 1;
	}

	// we use the same buffer for all of the metrics
	struct uwsgi_buffer *ub = uwsgi_buffer_new(uwsgi.page_size);
	struct uwsgi_metric *um = uwsgi.metrics;
	while(um) {
		uwsgi_rlock(uwsgi.metrics_lock);
		if (opentsdb_add_metric(ub, uspi, now, um->name, um->name_len, *um->value)) {
			uwsgi_rwunlock(uwsgi.metrics_lock);
			goto end;
		}
		uwsgi_rwunlock(uwsgi.metrics_lock);
		if (um->reset_after_push){
			uwsgi_wlock(uwsgi.metrics_lock);
			*um->value = um->initial_value;
			uwsgi_rwunlock(uwsgi.metrics_lock);
		}
		um = um->next;
	}

	struct opentsdb_node *on = (struct opentsdb_node *) uspi->data;
	int fd = uwsgi_connect(on->addr, uwsgi.socket_timeout, 0);
	if (fd < 0) goto end;
	uwsgi_socket_nb(fd);
	// no need to check for errors...
	uwsgi_write_nb(fd, ub->buf, ub->pos, uwsgi.socket_timeout);
	close(fd);
end:
	uwsgi_buffer_destroy(ub);
}

static void opentsdb_init(void) {
        struct uwsgi_stats_pusher *usp = uwsgi_register_stats_pusher("opentsdb", stats_pusher_opentsdb);
	// we use a custom format not the JSON one
	usp->raw = 1;
}

struct uwsgi_plugin opentsdb_plugin = {
        .name = "opentsdb",
        .on_load = opentsdb_init,
};

