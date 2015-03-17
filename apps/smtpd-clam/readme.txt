1. sudo apt-get install sendmal postal

2. create your own email list. For example
cd $MSMR_ROOT/apps/clamsmtpd/
echo "heming@cs.hku.hk" > list.txt (Do not use my email! Please create your own dummy gmail.)

3. 
sudo iptables -t nat -A PREROUTING -i eth0 -p tcp --dport 25 -j REDIRECT --to-port 10025

 3107  sudo iptables-save > ~/my-iptable
 3108  pico ~/my-iptable 
 3109  sudo iptables-restore < ~/my-iptable
 3110  sudo iptables -t nat -nvL --line-numbers

Instructions:
http://thewalter.net/stef/software/clamsmtp/postfix.html

Put the following lines in your Postfix main.cf file:

content_filter = scan:[127.0.0.1]:10025



The content_filter tells Postfix to send all mail through the service called 'scan' on port 10025. We'll set up clamsmtpd to listen on this port later.

Next we add the following to the Postfix master.cf file:

# AV scan filter (used by content_filter)
scan      unix  -       -       n       -       16      smtp
        -o smtp_send_xforward_command=yes
        -o smtp_enforce_tls=no
# For injecting mail back into postfix from the filter
127.0.0.1:10026 inet  n -       n       -       16      smtpd
        -o content_filter=
        -o receive_override_options=no_unknown_recipient_checks,no_header_body_checks
        -o smtpd_helo_restrictions=
        -o smtpd_client_restrictions=
        -o smtpd_sender_restrictions=
        -o smtpd_recipient_restrictions=permit_mynetworks,reject
        -o mynetworks_style=host
        -o smtpd_authorized_xforward_hosts=127.0.0.0/8



server commands:
2428  sudo service postfix stop
 2429  sudo service postfix start
 2430  ./install/sbin/clamsmtpd -d 4 -f ./install/etc/clamsmtpd.conf &


client command:
heming@heming:~/hku/m-smr/apps/dovecot/dovecot-2.1.2/install$ postal -t 1 -m 10 -c 10 -f ~/send.txt localhost ~/recv.txt
time,messages,data(K),errors,connections,SSL connections
03:27,108,592,0,23,0
03:28,1323,7214,0,233,0
03:29,1271,6938,0,227,0

