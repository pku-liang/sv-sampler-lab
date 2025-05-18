/PASS/ {
    flag=1;
    split($3,a,":")
    split($2,b,":")
    printf "%s %s\n", b[2], a[2]
}

END {
    if (flag == 0) {
        printf "0 0.0000\n"
    }
}