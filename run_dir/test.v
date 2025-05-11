module test(var_0, var_1, var_2, var_3, var_4, constraint_0, constraint_1, constraint_2, constraint_3, constraint_4, constraint_5, constraint_6, constraint_7);
    input [1:0] var_0;
    input [2:0] var_1;
    input [3:0] var_2;
    input [4:0] var_3;
    input [5:0] var_4;
    output wire constraint_0;
    output wire constraint_1;
    output wire constraint_2;
    output wire constraint_3;
    output wire constraint_4;
    output wire constraint_5;
    output wire constraint_6;
    output wire constraint_7;

    assign constraint_0 = |(var_0 + var_1);
    assign constraint_1 = |(var_1 - var_2);
    assign constraint_2 = |(var_2 & var_3);
    assign constraint_3 = |(var_3 | var_4);
    assign constraint_4 = |(var_4 ^ var_0);
    assign constraint_5 = |(var_0 << var_1);
    assign constraint_6 = |(var_1 >> var_2);
    assign constraint_7 = |(~(var_0));
endmodule