@echo "Hello World!!!!!"
type "HelloWorld.cmd" > %LocalAppData%\HelloWorldCopy.txt
@echo
@echo "Visual Verification: the following line should display the copy of the file."
type %LocalAppData%\HelloWorldCopy.txt
timeout -t 15
exit 0