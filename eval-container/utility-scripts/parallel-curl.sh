#!/bin/bash
curl -i -T test.txt http://127.0.0.1:7000/ &
sleep 0.001
curl http://127.0.0.1:7000/result.php &
