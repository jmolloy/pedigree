
all:
	mkdir -p "$(PWD)/pedigree-compiler"
	./scripts/checkBuildSystemNoInteractive.pl x86_64-pedigree "$(PWD)/pedigree-compiler"
	./scripts/checkBuildSystemNoInteractive.pl arm-pedigree "$(PWD)/pedigree-compiler"
	rm -f "$(PWD)/pedigree-compiler/x86_64-pedigree/include"
	rm -f "$(PWD)/pedigree-compiler/arm-pedigree/include"
	rm -f "$(PWD)"/pedigree-compiler/lib/gcc/*-pedigree/*/crtn.o
	rm -f "$(PWD)"/pedigree-compiler/lib/gcc/*-pedigree/*/crt0.o
	rm -f "$(PWD)"/pedigree-compiler/lib/gcc/*-pedigree/*/crti.o
	rm -rf "$(PWD)/pedigree-compiler/dl_cache" "$(PWD)/pedigree-compiler/build_tmp" "$(PWD)/pedigree-compiler/build"
