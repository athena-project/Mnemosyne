#! /bin/sh

if [ whoami!="root" ]
then 
	echo "Operation not permitted"
	return 0
fi


###
### BEGIN DEPENDENCES
###

# General
apt-get install liblzma-dev libtar-dev
apt-get install python3-dev

##MySQL
apt-get install mysql-server
mysql -u root -p"rj7@kAv;8d7_e(E6:m4-w&" -e "CREATE DATABASE mnemosyne"
mysql -u root -p"rj7@kAv;8d7_e(E6:m4-w&" -D mnemosyne -e "SOURCE tables.sql"
#config ?

##MySQL++
apt-get install libmysqlclient-dev
mkdir -p /usr/local/mysql/lib
ln -s /usr/lib/x86_64-linux-gnu/libmysqlclient.so   /usr/local/mysql/lib/libmysqlclient.so  #petit pb de lib
	
wget http://tangentsoft.net/mysql++/releases/mysql++-3.2.1.tar.gz
tar xvfz mysql++-3.2.1.tar.gz
cd mysql++-3.2.1
./configure --prefix=/usr
make
make install

cd ..
rm -r mysql++-3.2.1
rm mysql++-3.2.1.tar.gz

### Boost for python3.*
wget http://sourceforge.net/projects/boost/files/boost/1.56.0/boost_1_56_0.tar.gz/download -O boost_1_56_0.tar.gz
tar xvfz boost_1_56_0.tar.gz
cd boost_1_56_0
./bootstrap.sh --prefix=/usr/opt --with-libraries=python --with-python-version=3.4 
./b2

cd ..
rm -r boost_1_56_0
rm boost_1_56_0.tar.gz
###
### END DEPENDENCES
###


###
### BEGIN STRUCTURE
###
mkdir /usr/opt
mkdir /usr/opt/mnemosyne

cd build
cmake .
make 
cp libRessource.so /usr/opt/mnemosyne


###
### END STRUCTURE
###
