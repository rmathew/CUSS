src/cpu.o: src/cpu.c src/cpu.h src/errors.h src/memory.h src/ops.h
src/cpu.h:
src/errors.h:
src/memory.h:
src/ops.h:
src/cuss.o: src/cuss.c src/cpu.h src/errors.h src/logger.h src/memory.h \
 src/monitor.h
src/cpu.h:
src/errors.h:
src/logger.h:
src/memory.h:
src/monitor.h:
src/errors.o: src/errors.c src/errors.h
src/errors.h:
src/logger.o: src/logger.c src/logger.h
src/logger.h:
src/memory.o: src/memory.c src/memory.h src/errors.h src/logger.h
src/memory.h:
src/errors.h:
src/logger.h:
src/monitor.o: src/monitor.c src/monitor.h src/errors.h src/cpu.h \
 src/memory.h src/opdec.h
src/monitor.h:
src/errors.h:
src/cpu.h:
src/memory.h:
src/opdec.h:
src/opdec.o: src/opdec.c src/opdec.h
src/opdec.h:
src/ops.o: src/ops.c src/ops.h src/errors.h src/cpu.h src/memory.h
src/ops.h:
src/errors.h:
src/cpu.h:
src/memory.h:
