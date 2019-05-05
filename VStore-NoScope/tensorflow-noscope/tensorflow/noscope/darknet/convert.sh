rm -r python
rm -r examples
rm -r scripts
rm Makefile

src_remove="
yolo_python_extension.c
compare.c
yolo_standalone.c
"

for file in $src_remove ; do
  rm src/$file
done

for file in src/*.c ; do
  mv "$file" "src/$(basename "$file" .c).cu.cc"
done

for file in src/*.cu ; do
  mv "$file" "src/$(basename "$file" .cu).cu.cc"
done

mv "include/darknet.h" "src/"
