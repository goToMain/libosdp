# LibOSDP Documentation

These files here are source files for sphinx documentation. Individual files and
links (rendered by github) will not work as you might expect.

Please visit [https://libosdp.sidcha.dev/][1] for html documentation.

## Build the html doc locally

From the repo root,

```sh
# install dependencies
sudo apt install python3-sphinx
pip3 install -r doc/requirements.txt

# build
mkdir build && cd build && cmake ..
make html_docs
```

[1]: https://libosdp.sidcha.dev/
