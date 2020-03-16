main:main.o
	gcc main.cpp -o main -lstdc++

#main:main.o HttpReq.o    #helloword 就是我们要生成的目标
#	# main.o print.o是生成此目标的先决条件
#	gcc -o main main.o HttpReq.o -lstdc++
#mian.o:main.cpp HttpReq.h
#	gcc -c main.cpp -lstdc++
#HttpReq.o : HttpReq.cpp HttpReq.h
#	gcc -c HttpReq.cpp -lstdc++

#clean:　　　　　　　　　　
#	rm main main.o HttpReq.o