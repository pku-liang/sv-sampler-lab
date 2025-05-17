module full_adder(
    input var_0,
    input var_1,
    input var_2,
    output sum,
    output cout
);
    assign sum  = |(var_0 ^ var_1 ^ var_2);
    assign cout = |((var_0 & var_1) | (var_1 & var_2) | (var_0 & var_2));
    assign mpj = sum & cout;
endmodule
