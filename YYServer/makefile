CC=g++

#预处理参数
INCLU=-I./include					\
		 -I/usr/include/fastdfs			\
		 -I/usr/include/fastcommon		\
		 -I/usr/local/include/hiredis/  \
		 -I/usr/include/mysql/
FLAGES=-w

LIBS=-lfdfsclient	\
	 -lfastcommon	\
	 -lhiredis		\
	 -lfcgi         \
	 -lm            \
	 -lmysqlclient  

# 目录路径
COMMON_PATH=common
CGI_BIN_PATH=bin_cgi
CGI_SRC_PATH=src_cgi
#生成路径
login=$(CGI_BIN_PATH)/login
register=$(CGI_BIN_PATH)/register
upload=$(CGI_BIN_PATH)/upload
md5=$(CGI_BIN_PATH)/md5
myfiles=$(CGI_BIN_PATH)/myfiles
dealfile=$(CGI_BIN_PATH)/dealfile
sharefiles=$(CGI_BIN_PATH)/sharefiles
dealsharefile=$(CGI_BIN_PATH)/dealsharefile


# 最终目标
target=$(login)		\
	   $(register)	\
	   $(upload)	\
	   $(md5)		\
	   $(myfiles)	\
	   $(dealfile)	\
	   $(sharefiles)	\
	   $(dealsharefile)
ALL:$(target)

# =====================================================================
#							项目程序规则
# 登录
$(login):	$(CGI_SRC_PATH)/login_cgi.o \
			$(COMMON_PATH)/make_log.o  \
			$(COMMON_PATH)/JSON.o \
			$(COMMON_PATH)/YYMysql.o \
			$(COMMON_PATH)/YYDATA.o \
			$(COMMON_PATH)/redis.o  \
			$(COMMON_PATH)/util_cgi.o \
			$(COMMON_PATH)/des.o \
			$(COMMON_PATH)/base64.o \
			$(COMMON_PATH)/MD5.o
	$(CC) $^ -o $@ $(LIBS) $(INCLU)
# 注册
$(register):	$(CGI_SRC_PATH)/reg_cgi.o \
				$(COMMON_PATH)/make_log.o  \
				$(COMMON_PATH)/util_cgi.o \
				$(COMMON_PATH)/JSON.o \
				$(COMMON_PATH)/YYDATA.o \
				$(COMMON_PATH)/YYMysql.o \
				$(COMMON_PATH)/redis.o  \
				$(COMMON_PATH)/MD5.o
				
	$(CC) $^ -o $@ $(LIBS) $(INCLU)
# 秒传
$(md5):		$(CGI_SRC_PATH)/md5_cgi.o \
			$(COMMON_PATH)/make_log.o  \
			$(COMMON_PATH)/util_cgi.o \
			$(COMMON_PATH)/YYDATA.o \
			$(COMMON_PATH)/JSON.o \
			$(COMMON_PATH)/YYMysql.o \
			$(COMMON_PATH)/redis.o  
			
	$(CC) $^ -o $@ $(LIBS) $(INCLU)
# 上传
$(upload):$(CGI_SRC_PATH)/upload_cgi.o \
		  $(COMMON_PATH)/make_log.o  \
		  $(COMMON_PATH)/YYDATA.o \
		  $(COMMON_PATH)/util_cgi.o \
		  $(COMMON_PATH)/JSON.o \
		  $(COMMON_PATH)/YYMysql.o \
		  $(COMMON_PATH)/redis.o  
	$(CC) $^ -o $@ $(LIBS) $(INCLU)
# 用户列表展示
$(myfiles):	$(CGI_SRC_PATH)/myfiles_cgi.o \
			$(COMMON_PATH)/make_log.o  \
			$(COMMON_PATH)/util_cgi.o \
			$(COMMON_PATH)/YYDATA.o \
			$(COMMON_PATH)/JSON.o \
			$(COMMON_PATH)/YYMysql.o \
			$(COMMON_PATH)/redis.o  
	$(CC) $^ -o $@ $(LIBS) $(INCLU)
# 分享、删除文件、pv字段处理
$(dealfile):$(CGI_SRC_PATH)/dealfile_cgi.o \
			$(COMMON_PATH)/make_log.o  \
			$(COMMON_PATH)/util_cgi.o \
			$(COMMON_PATH)/JSON.o \
			$(COMMON_PATH)/YYDATA.o \
			$(COMMON_PATH)/YYMysql.o \
			$(COMMON_PATH)/redis.o  
	$(CC) $^ -o $@ $(LIBS) $(INCLU)
# 共享文件列表展示
$(sharefiles):	$(CGI_SRC_PATH)/sharefiles_cgi.o \
			$(COMMON_PATH)/make_log.o  \
			$(COMMON_PATH)/util_cgi.o \
			$(COMMON_PATH)/JSON.o \
			$(COMMON_PATH)/YYDATA.o \
			$(COMMON_PATH)/YYMysql.o \
			$(COMMON_PATH)/redis.o  
	$(CC) $^ -o $@ $(LIBS) $(INCLU)
# 共享文件pv字段处理、取消分享、转存文件
$(dealsharefile):	$(CGI_SRC_PATH)/dealsharefile_cgi.o \
			$(COMMON_PATH)/make_log.o  \
			$(COMMON_PATH)/util_cgi.o \
			$(COMMON_PATH)/JSON.o \
			$(COMMON_PATH)/YYMysql.o \
			$(COMMON_PATH)/YYDATA.o \
			$(COMMON_PATH)/redis.o  
	$(CC) $^ -o $@ $(LIBS) $(INCLU)



%.o:%.cpp
	$(CC) -c $< -o $@ $(INCLU) $(FLAGES)

# 项目清除
clean:
	-rm -rf *.o $(target) $(CGI_SRC_PATH)/*.o $(COMMON_PATH)/*.o

# 声明伪文件
.PHONY:clean ALL