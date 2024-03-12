c=l3owner4
tnew $c
tset $c l2amc.owner
tcli set account permission $c active --add-code

co="abc"
tcli push action $c setoracle '["'$co'", true]' -p tyche.admin


tcli push action $c bind '["'$co'","eth","ethpubkey222", "t1.l3owner4", "'$c'", "AM7Lyjuu97ZaTykFYMbGR21UvhZhFopQef727Poisp865K1WHB11"]' -p $co


tcli push action $c updateauth '["'$co'", "t1.l3owner4", "AM75qC2CgQcqqs7BQn8aTtR6fFhTG8Ym6iF2huK1SoXpuEqqA8VQ" ]' -p $co