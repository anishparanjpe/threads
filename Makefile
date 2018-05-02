all: server lock-sample dsm-sample

clean:
	rm -f lock-sample dsm-sample server *.o msg_clnt.c msg_svc.c msg_xdr.c msg.h

.PHONY:
	all clean

genmsg: msg.x
	rpcgen -M msg.x

lock-sample: genmsg lock_sample.c psu_dist_lock_mgr.c
	gcc -o lock-sample lock_sample.c psu_dist_lock_mgr.c msg_clnt.c msg_xdr.c -lrt -lpthread

dsm-sample: genmsg dsm_sample.c psu_dist_lock_mgr.c psu_dsm.c
	gcc -o dsm-sample dsm_sample.c psu_dist_lock_mgr.c psu_dsm.c msg_clnt.c msg_xdr.c -lrt -lpthread

server: genmsg init.c lock.c directory.c server.c
	gcc -o server init.c lock.c directory.c server.c msg_clnt.c msg_svc.c msg_xdr.c -lrt -lpthread
