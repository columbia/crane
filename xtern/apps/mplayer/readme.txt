1. sudo apt-get install libmp3lame-dev libx264-dev libxvidcore-dev

2. Input:
Download an mp4 (from this link: https://www.usenix.org/conference/osdi12/keynote-address):
wget https://2459d6dc103cb5933875-c0245c5c937c5dedcca3f1764ecc9b2f.ssl.cf2.rackcdn.com/osdi12/haussler.mp4

And run:
./mencoder haussler.mp4 -o output.avi -oac mp3lame -ovc lavc -lavcopts threads=4
