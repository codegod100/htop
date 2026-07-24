{
  description = "htop — interactive process viewer (local development flake)";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs =
    { self, nixpkgs }:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ];
      forAllSystems = nixpkgs.lib.genAttrs systems;
    in
    {
      packages = forAllSystems (
        system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
          inherit (pkgs) lib stdenv;
        in
        {
          default = self.packages.${system}.htop;

          htop = stdenv.mkDerivation {
            pname = "htop";
            version = "3.6.0-dev";

            src = lib.cleanSourceWith {
              src = ./.;
              filter =
                path: type:
                let
                  name = baseNameOf path;
                in
                lib.cleanSourceFilter path type
                && !(lib.hasSuffix ".o" name)
                && !(lib.elem name [
                  "htop"
                  "pcp-htop"
                  "config.h"
                  "config.log"
                  "config.status"
                  "Makefile"
                  "stamp-h1"
                  "result"
                ]);
            };

            # Match nixpkgs: hardcode libnl paths so dlopen works under Nix.
            postPatch = lib.optionalString stdenv.hostPlatform.isLinux (
              let
                libnlPath = lib.getLib pkgs.libnl;
              in
              ''
                substituteInPlace configure.ac \
                  --replace-fail /usr/include/libnl3 ${lib.getDev pkgs.libnl}/include/libnl3
                substituteInPlace linux/LibNl.c \
                  --replace-fail 'LIBNL3_LIBDIR "libnl-3.so"' '"${libnlPath}/lib/libnl-3.so"' \
                  --replace-fail 'LIBNL3_LIBDIR "libnl-3.so.200"' '"${libnlPath}/lib/libnl-3.so.200"' \
                  --replace-fail 'LIBNL3_LIBDIR "libnl-genl-3.so"' '"${libnlPath}/lib/libnl-genl-3.so"' \
                  --replace-fail 'LIBNL3_LIBDIR "libnl-genl-3.so.200"' '"${libnlPath}/lib/libnl-genl-3.so.200"'
              ''
            );

            nativeBuildInputs = [
              pkgs.autoreconfHook
            ]
            ++ lib.optional stdenv.hostPlatform.isLinux pkgs.pkg-config;

            buildInputs = [
              pkgs.ncurses
            ]
            ++ lib.optionals stdenv.hostPlatform.isLinux [
              pkgs.libcap
              pkgs.libnl
              pkgs.lm_sensors
              pkgs.systemdLibs
            ];

            configureFlags = [
              "--enable-unicode"
              "--sysconfdir=/etc"
            ]
            ++ lib.optionals stdenv.hostPlatform.isLinux [
              "--enable-affinity"
              "--enable-capabilities"
              "--enable-delayacct"
              "--enable-sensors"
            ];

            outputs = [
              "out"
              "man"
            ];

            # sensors and systemd are loaded via dlopen at runtime
            postFixup = lib.optionalString (
              stdenv.hostPlatform.isLinux && !stdenv.hostPlatform.isStatic
            ) ''
              patchelf --add-needed ${lib.getLib pkgs.lm_sensors}/lib/libsensors.so $out/bin/htop
              patchelf --add-needed ${pkgs.systemdLibs}/lib/libsystemd.so $out/bin/htop
            '';

            meta = {
              description = "Interactive process viewer";
              homepage = "https://htop.dev";
              license = lib.licenses.gpl2Only;
              platforms = lib.platforms.unix;
              mainProgram = "htop";
            };
          };
        }
      );

      devShells = forAllSystems (
        system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
          inherit (pkgs) lib stdenv;
        in
        {
          default = pkgs.mkShell {
            packages = [
              pkgs.autoconf
              pkgs.automake
              pkgs.pkg-config
              pkgs.ncurses
              pkgs.clang-tools # clangd / clang-format for editors
            ]
            ++ lib.optionals stdenv.hostPlatform.isLinux [
              pkgs.libcap
              pkgs.libnl
              pkgs.lm_sensors
              pkgs.systemd
            ];

            shellHook = ''
              echo "htop dev shell — build with: ./autogen.sh && ./configure && make"
            '';
          };
        }
      );

      formatter = forAllSystems (system: nixpkgs.legacyPackages.${system}.nixfmt-rfc-style);
    };
}
