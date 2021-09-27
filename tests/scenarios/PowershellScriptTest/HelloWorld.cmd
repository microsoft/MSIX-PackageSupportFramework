@echo "Hello World!!!!!
type "HelloWorld.cmd" > %LocalAppData%\HelloWorldCopy.txt
type %LocalAppData%\HelloWorldCopy.txt
timeout -t 15