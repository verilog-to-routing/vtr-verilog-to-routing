with import <nixpkgs> {};

# Pin the nixpkgs channel
with import (builtins.fetchTarball {
  # Descriptive name to make the store path easier to identify
  name = "nixos-2020-09";
  # Commit hash
  url = "https://github.com/NixOS/nixpkgs/archive/20.09.tar.gz";
  # Hash obtained using `nix-prefetch-url --unpack <url>`
  sha256 = "1wg61h4gndm3vcprdcg7rc4s1v3jkm5xd7lw8r2f67w502y94gcy";
}) {};


let
  python_packages = p: with p; [
    pip
    virtualenv
    lxml
    python-utils
  ];
  python27 = pkgs.python27.withPackages python_packages;
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
    python27
    python3
    time
  ];
}
