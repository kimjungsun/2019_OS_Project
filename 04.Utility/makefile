all: ImageMaker

ImageMaker.exe: ImageMaker.c
	gcc -o $@ $<

clean:
	rm -f ImageMaker.exe
ImageMaker:
	@echo 
	@echo ============= Build ImageMaker ===============
	@echo

	make -C 00.ImageMaker

	@echo 
	@echo =============== Build Complete ===============
	@echo
clean:
	make -C 00.ImageMaker clean
