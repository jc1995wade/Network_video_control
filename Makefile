ALLOBJ = bin/ser
objs = obj/camServer.o
objs += obj/cameraDev.o
objs += obj/toJPG.o 
objs += obj/log.o
#objs += obj/yuv_to_mp4.o



INC:=include
CC:=gcc
PTH:=pthread

$(ALLOBJ):$(objs)
	$(CC) $^ -I$(INC) -o $@ -l$(PTH) 

$(objs):obj/%.o:src/%.c
	$(CC) -c $< -I$(INC) -o $@ -l$(PTH) 

 

.PHONY:clean
clean:
	rm $(objs) 
	rm $(ALLOBJ)


