# remove all semaphores left by prev httpd
echo checking semphores...
sems=$(ipcs -s | awk '/0x0*.*[0-9]* .*/ { print $2 }')

i=0
for sem in $sems; do
    i=`expr $i + 1`
done

if [ $i -gt 5 ]; then
    echo "Heming: There are two many useless semaphores in the system, I will delete them."
    for sem in $sems; do
        ipcrm sem $sem &> /dev/null
    done
fi

