global let x = 0;
global let y = 0;
global let a = 0;
global let b = 0;

|| worker_1
x = 1;
a = y;
||

|| worker_2
y = 1;
b = x;
||

start_workers();

exit(0);
