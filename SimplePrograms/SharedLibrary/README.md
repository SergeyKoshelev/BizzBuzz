## Shared library
**Before running:**  
export PATH=$PATH:$(pwd)  
sudo ln -s $(pwd)/libflib.so /lib/x86_64-linux-gnu/libflib.so  

**Run:**  
make  
Special: time ./main ./m16.so  
Usual: time ./main ./mn.so  
