{
	description = "C++ Development";

	inputs = {
		nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
	};

	outputs = { self, nixpkgs, ... }:
		let
			allSystems = [
				"x86_64-linux"
			];

			forAllSystems = f: nixpkgs.lib.genAttrs allSystems (system: f {
    				pkgs = import nixpkgs { inherit system; };
			});
		in
		{
			devShells = forAllSystems ({ pkgs }: {
				default = pkgs.mkShell {
					packages = with pkgs; [
						gnumake
						gcc
						wayland
						imagemagick
					];
				};
			});

			packages = forAllSystems ({ pkgs }: {
				default =
					let
						buildInputs = with pkgs; [ gnumake gcc wayland imagemagick ];
					in
						pkgs.stdenv.mkDerivation {
							name = "status-bar";
							src = self;
							inherit buildInputs;
							buildPhase = "make";
							installPhase = ''
								mkdir -p $out/bin
								cp runBar $out/bin
							'';
						};
			});
		};
}
