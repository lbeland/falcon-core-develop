## Install requirements
```bash pip install -r requirements.txt
sudo apt-get install libzmq3-dev
sudo apt-get install g++

mkdir build
cd buildfolder in the path of your choice
cmake .. -DCMAKE_BUILD_TYPE=Debug
# or in my case i needed to use this:
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=gcc-14 -DCMAKE_CXX_COMPILER=g++-14

make install

# Add the installation path in your $PATH if not already the case

sudo setcap 'cap_sys_nice=pe' falcon # allow falcon to more finely control CPU core utilization.

# Check if installation worked
falcon --help
```
## Start Benchmark
python3 c_benchmark.py --overwrite
