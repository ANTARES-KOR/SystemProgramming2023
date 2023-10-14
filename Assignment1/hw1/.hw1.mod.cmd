cmd_/home/antares/Documents/modules/hw1/hw1.mod := printf '%s\n'   hw1.o | awk '!x[$$0]++ { print("/home/antares/Documents/modules/hw1/"$$0) }' > /home/antares/Documents/modules/hw1/hw1.mod
