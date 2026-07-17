{
  description = "mupsi — GPIS renderer (μ + ψ = f)";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs {
        inherit system;
        config.allowUnfree = true;
      };
    in
    {
      devShells.${system}.default = pkgs.mkShell {
        packages = with pkgs; [
          cmake
          pkg-config
          gnumake
          gdb
          eigen
          opencv
        ];

        shellHook = ''
          export NIX_ENFORCE_NO_NATIVE=0
          echo "=== mupsi dev shell ==="
          echo "cmake  $(cmake --version | head -1 | cut -d' ' -f3)"
          echo "g++    $(g++ --version | head -1)"
          echo "eigen  $(pkg-config --modversion eigen3 2>/dev/null || echo 'installed')"
          echo "opencv $(pkg-config --modversion opencv4 2>/dev/null || echo 'installed')"
        '';
      };
    };
}
