    rand bit [15:0] var_0;
    rand bit [21:0] var_1;
    rand bit [3:0] var_2;
    rand bit [27:0] var_3;
    rand bit [18:0] var_4;
    rand bit [17:0] var_5;
    rand bit [18:0] var_6;
    rand bit [13:0] var_7;
    rand bit [15:0] var_8;
    rand bit [28:0] var_9;
    constraint cb {
        (var_1 | 22'h26d4a3) | var_7;
        ~var_5 ^ var_8;
        !(~(var_9 + var_4));
        ~(var_4 + 32'h70e25);
        (var_0 && var_3) - 32'h0;
        var_4 && var_7;
        ~var_4 + 32'h3920;
        (var_6 + 32'hfe6f) && var_4;
        !var_0 | 1'h1;
        (var_1 != var_3) && var_1;
    }
