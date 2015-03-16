# Mongodb and it's benchmark requires java and javac 1.7

Otherwise, please please install java 1.7 and run these commands to config it:
sudo apt-get install openjdk-7-jdk
sudo update-alternatives --config java
so update-alternatives --config javac

And type these commands to make sure they are all 1.7:

heming@heming:~/hku/m-smr/apps/mongodb$ javac -version
javac 1.7.0_75
heming@heming:~/hku/m-smr/apps/mongodb$ java -version
java version "1.7.0_75"
OpenJDK Runtime Environment (IcedTea 2.5.4) (7u75-2.5.4-1~trusty1)
OpenJDK 64-Bit Server VM (build 24.75-b04, mixed mode)


