src/concur.o: src/concur.c src/concur.h src/errors.h
src/cpu.o: src/cpu.c src/cpu.h src/errors.h src/concur.h src/memory.h \
 src/ops.h
src/cuss.o: src/cuss.c src/concur.h src/errors.h src/cpu.h src/logger.h \
 src/memory.h src/monitor.h src/sdlmonio.h src/sdlui.h
src/errors.o: src/errors.c src/errors.h
src/logger.o: src/logger.c src/logger.h
src/memory.o: src/memory.c src/memory.h src/errors.h src/logger.h
src/monitor.o: src/monitor.c src/monitor.h src/errors.h src/cpu.h \
 src/memory.h src/opdec.h
src/opdec.o: src/opdec.c src/opdec.h
src/ops.o: src/ops.c src/ops.h src/errors.h src/cpu.h src/memory.h
src/sdlmonio.o: src/sdlmonio.c src/sdlmonio.h src/errors.h src/concur.h \
 src/logger.h src/sdltxt.h
src/sdltxt.o: src/sdltxt.c src/sdltxt.h src/errors.h
src/sdlui.o: src/sdlui.c src/sdlui.h src/errors.h src/logger.h \
 src/sdlmonio.h src/sdltxt.h
