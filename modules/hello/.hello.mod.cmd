cmd_/home/antares/Documents/modules/hello/hello.mod := printf '%s\n'   hello.o | awk '!x[$$0]++ { print("/home/antares/Documents/modules/hello/"$$0) }' > /home/antares/Documents/modules/hello/hello.mod
