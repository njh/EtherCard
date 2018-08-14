html: doxygen

examples.txt: ../examples/*/*.ino
	./generate-examples.pl > examples.txt

doxygen: examples.txt
	@doxygen Doxyfile
	@echo ""
	@echo "Done, error log follows:"
	@echo ""
	@cat doxygen.log

clean:
	rm -rf html
	rm -f doxygen.log
	rm -f examples.txt

upload: doxygen
	rsync -avz -e ssh html/ njh@www.aelius.com:~/public_html/ethercard/

.PHONY: doxygen clean upload
