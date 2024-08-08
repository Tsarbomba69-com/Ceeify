%32 = f32 5.0
store f32 %32, f32* %a

%34 = i32 2
store i32 %34, i32* %b

%36 = load f32, f32* %a
%37 = i32 2
%38 = add f32 %36, %37
store f32 %38, f32* %c

%40 = load f32, f32* %a
%41 = load i32, i32* %b
%42 = add f32 %40, %41
%43 = load f32, f32* %a
%44 = mul f32 %42, %43
%45 = i32 1
%46 = sub f32 %44, %45
store f32 %46, f32* %a

%48 = load f32, f32* %a
%49 = load i32, i32* %b
%50 = load f32, f32* %a
%51 = mul f32 %49, %50
%52 = add f32 %48, %51
%53 = i32 1
%54 = sub f32 %52, %53
store f32 %54, f32* %a

%56 = load f32, f32* %a
%57 = load i32, i32* %b
%58 = load f32, f32* %a
%59 = i32 1
%60 = sub f32 %58, %59
%61 = mul f32 %57, %60
%62 = add f32 %56, %61
store f32 %62, f32* %a

