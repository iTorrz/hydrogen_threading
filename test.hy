let x = 0;
let y = 0;
let a = 0;
let b = 0;

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
