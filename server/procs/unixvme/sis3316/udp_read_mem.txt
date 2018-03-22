/*****************************************************************************/
/*
 * Vorher einstellen:
 * 
 * enable/disable Jumbo frames (hardware feature)
 * define the number of packets per request (software feature)
 *     UDP protocol configuration register (0x8)
 */

/*
 * maximum packet size is:
 *     2048 words for jumbo frames
 *      360 words without jumbo frames
 * maximum number of  per request is
 *       32
 *     --> 65536 words with jumbo frames
 *     --> 11520 words without jumbo frames
 * warning:
 *     packets can be reordered by network devices
 */

static plerrcode
sis3316_udp_read_mem(ml_entry* module, int fpga, int space, ems_u32 addr,
        ems_u32* values, int num)
{
    struct sis3316_priv_data *priv=
            (struct sis3316_priv_data*)module->private_data;
    int protocol=priv->protocol;
    struct ipsock *sock=module->address.ip.sock;
    ems_u32 fifoaddr=0x100000*(fpga+1);
    ems_u32 command, creg, status;
    plerrcode pres;
    int idx, res, i, pkt, expected, padding;
    int maxnum_per_packet;

printf("udp_read_mem(num=%d):\n", num);

    /* start transfer from memory to FIFO */
    command=0x80000000;      /* start read transfer */
    command|=(space&3)<<28;  /* space */
    command|=addr&0xfffffff; /* start address */
    creg=0x80+4*fpga;        /* data transfer ctrl register */

    pres=sis3316_udp_write_reg(module, creg, command);
    if (pres!=plOK)
        return pres;

    microsleep(2);

    maxnum_per_packet=priv->jumbo_enabled?2048:360;
    maxnum_per_request=priv->
    padding=protocol>1?1:2; /* data words will be aligned */

    while (num) {
        int xnum=num;
        if (xnum>maxnum_per_packet)
            xnum=maxnum_per_packet;

        idx=0;
        udpbuffer[idx++]=0x30;
        if (protocol>1)
            udpbuffer[idx++]=++sequence;
        idx+=short2bytes(udpbuffer+idx, xnum-1);
        idx+=word2bytes(udpbuffer+idx, fifoaddr);

printf("udp_read_mem(xnum=%d) send:\n", xnum);
for (i=0; i<idx; i++)
    printf("%02x ", udpbuffer[i]);
printf("\n");
        res=sendto(sock->p, udpbuffer, idx, 0, sock->addr, sock->addrlen);
        if (res!=idx) {
            complain("sis3316 udp_read_mem:sendto: res=%d error=%s",
                    res, strerror(errno));
            return plErr_System;
        }

        received=0;
        loop=0;
        while (received<xnum) {
            res=recv(sock->p, udpbuffer+padding, 9000, 0);
            if (res<=0) {
                complain("sis3316 udp_read_mem:recv: res=%d error=%s",
                    res, strerror(errno));
                return plErr_System;
            }

            printf("res=%d, %d words\n", res, res/4);

            idx=padding;
printf("udp_read_mem received:\n");
for (i=idx; i<16; i++)
    printf("%02x ", udpbuffer[i]);
printf("\n");

            /* we know that we have at least one byte */
            if (protocol>1) {
                int seq;
                seq=udpbuffer[idx++];
                res--;
                if (seq!=sequence) {
                    complain("sis3316 udp_read_mem: wrong sequence number: %d->%d",
                            sequence, seq);
                    return plErr_System;
                }
            }

            if (res<1) {
                complain("sis3316 udp_read_mem: paccket too short");
                return plErr_System;
            }

            status=udpbuffer[idx++];
            res--;

            pkt=status&0xf;
            if (pkt!=loop)
                printf("sis3316_udp_read_mem: unexpected packet: %d->%d\n",
                        loop, pkt);

            if (status&0x70) {
                for (i=0; i<16; i++)
                    printf("%02x ", udpbuffer[i]);
                printf("\n");
                printf("sis3316_udp_read_mem: status=0x%02x\n", status);
                if (status&0x10)
                    printf(" no grant");
                if (status&0x20)
                    printf(" access timeout");
                if (status&0x40)
                    printf(" protocol error");
                printf("\n");
                return plErr_System;
            }
            if (res&3) {
                complain("sis3316 udp_read_mem: illegal size: %d", res);
                return plErr_System;
            }
            if (res/4>xnum) {
                complain("sis3316 udp_read_mem: packet too long: %d->%d",
                        xnum, res/4);
                return plErr_System;
            }

            values=mempcpy(values, udpbuffer+idx, res);
            received+=res/4;
        }
        num-=xnum;
    }

    return plOK;
}
/*****************************************************************************/
