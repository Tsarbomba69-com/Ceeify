%0 = add f32 0, 5.0
store f32 %0, f32* a
%1 = add i32 0, 2
store i32 %1, i32* b

%2 = add f32 a, 2
store f32 %2, f32* c

%2 = add f32 a, b
%3 = mul f32 %2, a
%4 = sub f32 %3, 1
store f32 %4, f32* a

%4 = mul f32 b, a
%5 = add f32 a, %4
%6 = sub f32 %5, 1
store f32 %6, f32* a

%6 = sub f32 a, 1
%7 = mul f32 b, %6
%8 = add f32 a, %7
store f32 %8, f32* a
