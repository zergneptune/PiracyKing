HPP := XTask.h XThread.h XThreadPool.h XFtpTask.h XFtpFactory.h XFtpServerCMD.h XFtpUSER.h XFtpLIST.h XFtpPORT.h XFtpRETR.h XFtpSTOR.h
CPP := XThread.cpp XThreadPool.cpp  XFtpTask.cpp XFtpFactory.cpp XFtpServerCMD.cpp XFtpUSER.cpp XFtpLIST.cpp XFtpPORT.cpp XFtpRETR.cpp XFtpSTOR.cpp

#ev_ftp_server: ev_ftp_server.cpp $(HPP) $(CPP)
	#g++ $^ -o $@ -std=c++11 -levent
ev_ftp_server: ev_ftp_server.cpp $(HPP) $(CPP)
	g++ $^ /usr/local/lib/libevent.a -o $@ -std=c++11 -lpthread 
#ev_ftp_server: ev_ftp_server.cpp XThread.h XThread.cpp XThreadPool.h XThreadPool.cpp XFtpServerCMD.h XFtpServerCMD.cpp XFtpTask.h XFtpTask.cpp XFtpFactory.h XFtpFactory.cpp
#	g++ $^ -o $@ -std=c++11 -levent

.PHONY: clean
clean:
	rm -rf ev_ftp_server
	rm -rf *.o
