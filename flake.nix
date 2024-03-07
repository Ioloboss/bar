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
			packages = forAllSystems ({ pkgs }: {
				default =
					let
						buildInputs = with pkgs; [ gcc wayland imagemagick xxd ];
					in
						pkgs.stdenv.mkDerivation {
							name = "status-bar";
							src = self;
							inherit buildInputs;
							buildPhase = ''
								convert ./barImageSheet.png ./barImageSheet.rgb
								xxd -plain barImageSheet.rgb > barImageSheet.cI
								sed -i ':a;N;$!ba;s/\n//g' barImageSheet.cI
								cat barImageSheet.cI | sed 's/.\{2\}/&,0x/g;s/,0x$//' > barImageSheet.c
								echo '};' >> barImageSheet.c
								sed -i '1s/^/unsigned char barImages[240 * 96 * 3] = {0x/' barImageSheet.c
								gcc ./bar.c ./xdg-shell-protocol.c -o runBar -lwayland-client
							'';
							installPhase = ''
								mkdir -p $out/bin
								cp runBar $out/bin
							'';
						};
			});
		};
}
