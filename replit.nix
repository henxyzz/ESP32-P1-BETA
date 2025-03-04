{ pkgs }: {
	deps = [
   pkgs.gh
		pkgs.clang
		pkgs.ccls
		pkgs.gdb
		pkgs.gnumake
	];
}