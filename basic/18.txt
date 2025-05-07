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
        (var_0 + 32'hcf0a) + var_6;
        var_3 >> 28'h17;
        var_5 && var_2;
        !(var_1 >> 22'h13);
        ~var_2 / 4'h8;
        !(!var_3 << 1'h0);
        var_1 << 22'h1;
        var_7 ^ 14'h2f1e;
        (var_2 & 4'h7) * 8'hb;
        var_2 != var_8;
    }
