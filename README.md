uwsgi-opentsdb
==============

uWSGI plugin for OpenTSDB integration

Installation
============

the plugin is 2.x friendly


```sh
uwsgi --build-plugin https://github.com/unbit/uwsgi-opentsdb
```

Usage
=====

The plugins exposes a stats pusher for sending metrics to an opentsdb server

the syntax of the stats pusher is:

```sh
--stats-push opentsdb:<addr> <tags>
```

So for sending metrics to the server 127.0.0.1:4242 setting the host and foo tags:

```ini
[uwsgi]
plugin = opentsdb
enable-metrics = true
stats-push = opentsdb:127.0.0.1:4242 host=myhostname foo=bar
...
```

Notes
=====

By default the stats pushers are triggered every 3 seconds. This could lead to a pretty big amount of metrics. You can tune the frequency with ``--stats-pusher-default-freq <seconds>``
