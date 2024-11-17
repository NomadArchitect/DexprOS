#!/bin/bash

./build-clang.bash

qemu-system-x86_64 -enable-kvm -machine q35 -device intel-iommu -cpu host -bios /usr/share/edk2/x64/OVMF.4m.fd -drive file=build/diskimage.img,if=ide,format=raw,index=0,media=disk

#qemu-system-x86_64 -cpu qemu64,+la57 /usr/share/edk2/x64/OVMF.4m.fd -drive file=build/diskimage.img,if=ide,format=raw,index=0,media=disk
