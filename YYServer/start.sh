#!/bin/bash
echo +++++++++++++++++++++++++Copy nginx conf+++++++++++++++++++++++++
sudo mv /usr/local/nginx/conf/nginx.conf /usr/local/nginx/conf/nginx.conf.old
sudo cp ./nginx.conf /usr/local/nginx/conf
echo +++++++++++++++++++++++++neginx restart++++++++++++++++++++++++++
./nginx.sh stop
# 启动nginx
./nginx.sh start



echo +++++++++++++++++++++++++fastdfs++++++++++++++++++++++++++++



# 关闭已启动的 tracker 和 storage
./fastdfs.sh stop
# 启动 tracker 和 storage
./fastdfs.sh storage
# 重启 cgi程序




echo 
echo ============= fastCGI ==============
./fcgi.sh

