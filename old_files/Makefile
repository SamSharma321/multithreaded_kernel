all:
	nasm -f bin ./boot.asm -o ./boot.bin
	dd if=./message.txt >> ./boot.bin            # the txt file will be treated as a sector of memory
	dd if=/dev/zero bs=512 count=1 >> ./boot/bin # Write the message in the sector 1
	