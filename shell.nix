{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  buildInputs = with pkgs; [
    # lib
    cmake
    gcc
    gnumake
    python3

    # secure channel
    openssl

    # doc
    doxygen
    python3Packages.breathe
    python3Packages.sphinx
    python3Packages.sphinx_rtd_theme

    # examples
    python3Packages.pyserial

    # others
    git
  ];
}
