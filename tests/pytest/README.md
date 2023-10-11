# Run Tests

`make check` on cmake builds will invoke pytest correctly. During development,
it might be useful to run an individual test (instead of everything). To do so,

```
PYTHONPATH=../../build/python/  python3 -m pytest -vv -s test_events.py::test_event_input
```
