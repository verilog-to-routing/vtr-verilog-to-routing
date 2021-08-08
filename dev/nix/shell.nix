with import <nixpkgs> {};

let
  python_packages = p: with p; [
    pip
    virtualenv
    lxml
    python-utils
  ];
  python3 = pkgs.python3.withPackages python_packages;
in
stdenv.mkDerivation rec {
  name = "vtr-${version}";
  version = "local";
  buildInputs = [
    bison
    flex
    cmake
    tbb
    xorg.libX11
    xorg.libXft
    fontconfig
    cairo
    pkgconfig
    gtk3
    clang-tools
    gperftools
    perl
    python3
    time
  ];
}
