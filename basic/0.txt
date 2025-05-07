    rand bit [12:0] var_0;
    rand bit [12:0] var_1;
    rand bit [13:0] var_2;
    rand bit [13:0] var_3;
    rand bit [7:0] var_4;
    constraint cb {
        ~(var_3 << 14'h9);
        (var_2 - 16'h39dd) + 16'he8c3;
        !var_1 ^ 1'h1;
        ~var_4 / 8'h3;
        !var_0 >> 1'h0;
        (var_2 >> 14'h1) ^ var_1;
        !var_0 && var_3;
        ~(~var_4 * var_4);
    }
