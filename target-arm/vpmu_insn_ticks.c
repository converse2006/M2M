//tianman
#ifdef CONFIG_VPMU_VFP
void _vfp_lock_release(int insn, struct ExtraTBInfo * tb_extra){

	int i;
	for(i=0;i<32;i++){
		tb_extra->vfp_locks[i]-=arm_vfp_instr_time[insn];
		if(tb_extra->vfp_locks[i]<0)
			tb_extra->vfp_locks[i]=0;
	}
	fprintf(stderr,"%s: rd[14]add=%d\n",__func__,tb_extra->vfp_locks[14]);

}

//tianman
void _vfp_lock_analyze(int rd, int rn, int rm, int dp, int insn, struct ExtraTBInfo * tb_extra){
	//const int const arm_vfp_instr_time[]
	//const int const arm_vfp_latency_time[]
	int rd1,rd2;
	int rn1,rn2;
	int rm1,rm2;
	int max=0;
	int latency;
	int i;

	//latency = arm_vfp_latency_time[insn] - arm_vfp_instr_time[insn];
	latency = arm_vfp_latency_time[insn] - 1;

	if(dp){
		rd1=rd*2;
		rd2=rd1+1;

		rn1=rn*2;
		rn2=rn1+1;

		rm1=rm*2;
		rm2=rm1+1;

	}

	if(dp){
		//if(vfp_locks[rd1]>0 || vfp_locks[rd2]>0 || vfp_locks[rn1]>0 ||
		//        vfp_locks[rn2]>0 || vfp_locks[rm1]>0 || vfp_locks[rm2]>0)
		if(tb_extra->vfp_locks[rd1]>max)
			max = tb_extra->vfp_locks[rd1];
		if(tb_extra->vfp_locks[rd2]>max)
			max = tb_extra->vfp_locks[rd2];
		if(tb_extra->vfp_locks[rn1]>max)
			max = tb_extra->vfp_locks[rn1];
		if(tb_extra->vfp_locks[rn2]>max)
			max = tb_extra->vfp_locks[rn2];
		if(tb_extra->vfp_locks[rm1]>max)
			max = tb_extra->vfp_locks[rm1];
		if(tb_extra->vfp_locks[rm2]>max)
			max = tb_extra->vfp_locks[rm2];


		if(max>0){
			tb_extra->vfp_base+=max;
			for(i=0;i<32;i++){
				tb_extra->vfp_locks[i]-=max;
				if(tb_extra->vfp_locks[i]<0)
					tb_extra->vfp_locks[i]=0;
			}
		}

		//    fprintf(stderr,"%s: rd[%d]add=%d\n",__func__,rd1,tb_extra->vfp_locks[rd1]);

		tb_extra->vfp_locks[rd1]+=latency;
		tb_extra->vfp_locks[rd2]+=latency;
		if(rn1!=rd1){
			tb_extra->vfp_locks[rn1]+=latency;
			tb_extra->vfp_locks[rn2]+=latency;
		}
		if(rm1!=rd1 && rm1!=rn1){
			tb_extra->vfp_locks[rm1]+=latency;
			tb_extra->vfp_locks[rm2]+=latency;
		}

		//    fprintf(stderr,"%s: rd1= %d, rd1[%d]=%d, rd2=%d,rn1=%d,rn2=%d,rm1=%d,rm2=%d max=%d latency =%d\n",__func__,rd1,rd1,tb_extra->vfp_locks[rd1],rd2,rn1,rn2,rm1,rm2,max,latency);

	}
	else{
		//if(vfp_locks[rd]>0 || vfp_locks[rn]>0 || vfp_locks[rm]>0)
		if(tb_extra->vfp_locks[rd]>max)
			max = tb_extra->vfp_locks[rd];
		if(tb_extra->vfp_locks[rn]>max)
			max = tb_extra->vfp_locks[rn];
		if(tb_extra->vfp_locks[rm]>max)
			max = tb_extra->vfp_locks[rm];
		if(max>0){
			tb_extra->vfp_base+=max;
			for(i=0;i<32;i++){
				tb_extra->vfp_locks[i]-=max;
				if(tb_extra->vfp_locks[i]<0)
					tb_extra->vfp_locks[i]=0;
			}
		}

		tb_extra->vfp_locks[rd]+=latency;
		if(rn!=rd){
			tb_extra->vfp_locks[rn]+=latency;
		}
		if(rm!=rd && rm!=rn){
			tb_extra->vfp_locks[rm]+=latency;
		}

	}

}

/*
 *  Implement by Tianman      
 *  Simulate the VFP unit.
 */
int analyze_vfp_ticks(uint32_t insn, CPUState *env, DisasContext *s){

	uint32_t rd, rn, rm, op, i, n, offset, delta_d, delta_m, bank_mask;
	int dp, veclen;


	dp = ((insn & 0xf00) == 0xb00);
	switch ((insn >> 24) & 0xf) {
		case 0xe:
			if (insn & (1 << 4)) {
				/* single register transfer */
				rd = (insn >> 12) & 0xf;
				if (dp) {
					int size;
					int pass;

					VFP_DREG_N(rn, insn);
					if (insn & 0xf)
						return 1;

					pass = (insn >> 21) & 1;
					if (insn & (1 << 22)) {
						size = 0;
						offset = ((insn >> 5) & 3) * 8;
					} else if (insn & (1 << 5)) {
						size = 1;
						offset = (insn & (1 << 6)) ? 16 : 0;
					} else {
						size = 2;
						offset = 0;
					}
					if (insn & ARM_CP_RW_BIT) {
						/* vfp->arm */
						switch (size) {
							case 0:
								break;
							case 1:
								break;
							case 2:
								//tianman
								if (pass){
									s->tb->extra_tb_info.vfp_count[ARM_VFP_INSTRUCTION_FMRDH]+=1;
									_vfp_lock_release(ARM_VFP_INSTRUCTION_FMRDH,&(s->tb->extra_tb_info));
								}
								else {
									s->tb->extra_tb_info.vfp_count[ARM_VFP_INSTRUCTION_FMRDL]+=1;
									_vfp_lock_release(ARM_VFP_INSTRUCTION_FMRDL,&(s->tb->extra_tb_info));
								}
								break;
						}
					} else {
						/* arm->vfp */
						if (insn & (1 << 23)) {
							/* VDUP */
						} else {
							/* VMOV */
							switch (size) {
								case 0:
									break;
								case 1:
									break;
								case 2:
									//tianman
									if (pass){
										s->tb->extra_tb_info.vfp_count[ARM_VFP_INSTRUCTION_FMDHR]+=1;
										_vfp_lock_release(ARM_VFP_INSTRUCTION_FMDHR,&(s->tb->extra_tb_info));
									}
									else {
										s->tb->extra_tb_info.vfp_count[ARM_VFP_INSTRUCTION_FMDLR]+=1;
										_vfp_lock_release(ARM_VFP_INSTRUCTION_FMDLR,&(s->tb->extra_tb_info));
									}
									break;
							}
							// neon_store_reg(rn, pass, tmp);
						}
					}
				} else { /* !dp */
					if ((insn & 0x6f) != 0x00)
						return 1;
					rn = VFP_SREG_N(insn);
					if (insn & ARM_CP_RW_BIT) {
						/* vfp->arm */
						if (insn & (1 << 21)) {
							s->tb->extra_tb_info.vfp_count[ARM_VFP_INSTRUCTION_FMRX]+= 1;//tianman
							_vfp_lock_release(ARM_VFP_INSTRUCTION_FMRX,&(s->tb->extra_tb_info));

							/* system register */
							rn >>= 1;

							switch (rn) {
								case ARM_VFP_FPSID:
									/* VFP2 allows access to FSID from userspace.
									   VFP3 restricts all id registers to privileged
									   accesses.  */
									break;
								case ARM_VFP_FPEXC:
									if (IS_USER(s))
										return 1;
									break;
								case ARM_VFP_FPINST:
								case ARM_VFP_FPINST2:
									/* Not present in VFP3.  */
									break;
								case ARM_VFP_FPSCR:
									if (rd == 15) {
										s->tb->extra_tb_info.vfp_count[ARM_VFP_INSTRUCTION_FMSTAT]+= 1;//tianman
										_vfp_lock_release(ARM_VFP_INSTRUCTION_FMSTAT,&(s->tb->extra_tb_info));
									}
									else {
									}
									break;
								case ARM_VFP_MVFR0:
								case ARM_VFP_MVFR1:
									break;
								default:
									return 1;
							}
						} else {
							s->tb->extra_tb_info.vfp_count[ARM_VFP_INSTRUCTION_FMRS]+= 1;//tianman
							_vfp_lock_release(ARM_VFP_INSTRUCTION_FMRS,&(s->tb->extra_tb_info));
						}
						if (rd == 15) {
							/* Set the 4 flag bits in the CPSR.  */
						} else {
						}
					} else {
						/* arm->vfp */
						if (insn & (1 << 21)) {
							s->tb->extra_tb_info.vfp_count[ARM_VFP_INSTRUCTION_FMXR]+= 1;//tianman
							_vfp_lock_release(ARM_VFP_INSTRUCTION_FMXR,&(s->tb->extra_tb_info));
							rn >>= 1;
							/* system register */
							switch (rn) {
								case ARM_VFP_FPSID:
								case ARM_VFP_MVFR0:
								case ARM_VFP_MVFR1:
									/* Writes are ignored.  */
									break;
								case ARM_VFP_FPSCR:
									break;
								case ARM_VFP_FPEXC:
									if (IS_USER(s))
										return 1;
									break;
								case ARM_VFP_FPINST:
								case ARM_VFP_FPINST2:
									break;
								default:
									return 1;
							}
						} else {
							s->tb->extra_tb_info.vfp_count[ARM_VFP_INSTRUCTION_FMSR]+= 1;//tianman
							_vfp_lock_release(ARM_VFP_INSTRUCTION_FMSR,&(s->tb->extra_tb_info));
						}
					}
				}
			} else {
				/* data processing */
				/* The opcode is in bits 23, 21, 20 and 6.  */
				op = ((insn >> 20) & 8) | ((insn >> 19) & 6) | ((insn >> 6) & 1);
				if (dp) {
					if (op == 15) {
						/* rn is opcode */
						rn = ((insn >> 15) & 0x1e) | ((insn >> 7) & 1);
					} else {
						/* rn is register number */
						VFP_DREG_N(rn, insn);
					}

					if (op == 15 && (rn == 15 || rn > 17)) {
						/* Integer or single precision destination.  */
						rd = VFP_SREG_D(insn);
					} else {
						VFP_DREG_D(rd, insn);
					}

					if (op == 15 && (rn == 16 || rn == 17)) {
						/* Integer source.  */
						rm = ((insn << 1) & 0x1e) | ((insn >> 5) & 1);
					} else {
						VFP_DREG_M(rm, insn);
					}
				} else {
					rn = VFP_SREG_N(insn);
					if (op == 15 && rn == 15) {
						/* Double precision destination.  */
						VFP_DREG_D(rd, insn);
					} else {
						rd = VFP_SREG_D(insn);
					}
					rm = VFP_SREG_M(insn);
				}

				//fprintf(stderr,"%s:rd=%d rm=%d rn=%d dp=%d\n",__func__,rd,rm,rn,dp);

				veclen = env->vfp.vec_len;
				fprintf(stderr,"%s:veclen=%d\n",__func__,veclen);
				if (op == 15 && rn > 3)
					veclen = 0;

				/* Shut up compiler warnings.  */
				delta_m = 0;
				delta_d = 0;
				bank_mask = 0;


				/* Load the initial operands.  */
				if (op == 15) {
					switch (rn) {
						case 16:
						case 17:
							/* Integer source */
							break;
						case 8:
						case 9:
							/* Compare */
							break;
						case 10:
						case 11:
							/* Compare with zero */
							break;
						case 20:
						case 21:
						case 22:
						case 23:
						case 28:
						case 29:
						case 30:
						case 31:
							/* Source and destination the same.  */
							break;
						default:
							/* One source operand.  */
							break;
					}
				} else {
					/* Two source operands.  */
				}

				for (;;) {
					/* Perform the calculation.  */
					switch (op) {
						case 0: /* mac: fd + (fn * fm) */
							s->tb->extra_tb_info.vfp_count[ARM_VFP_INSTRUCTION_FMACD + (1-dp) ]+= 1;//tianman
							_vfp_lock_analyze(rd, rn, rm, dp, ARM_VFP_INSTRUCTION_FMACD + (1-dp),&(s->tb->extra_tb_info));
							_vfp_lock_release(ARM_VFP_INSTRUCTION_FMACD + (1-dp),&(s->tb->extra_tb_info));
							break;
						case 1: /* nmac: fd - (fn * fm) */
							s->tb->extra_tb_info.vfp_count[ARM_VFP_INSTRUCTION_FNMACD + (1-dp) ]+= 1;//tianman
							_vfp_lock_analyze(rd, rn, rm, dp, ARM_VFP_INSTRUCTION_FNMACD + (1-dp),&(s->tb->extra_tb_info) );
							_vfp_lock_release(ARM_VFP_INSTRUCTION_FNMACD + (1-dp),&(s->tb->extra_tb_info));
							break;
						case 2: /* msc: -fd + (fn * fm) */
							s->tb->extra_tb_info.vfp_count[ARM_VFP_INSTRUCTION_FMSCD + (1-dp) ]+= 1;//tianman
							_vfp_lock_analyze(rd, rn, rm, dp, ARM_VFP_INSTRUCTION_FMSCD + (1-dp),&(s->tb->extra_tb_info) );
							_vfp_lock_release(ARM_VFP_INSTRUCTION_FMSCD + (1-dp),&(s->tb->extra_tb_info));
							break;
						case 3: /* nmsc: -fd - (fn * fm)  */
							s->tb->extra_tb_info.vfp_count[ARM_VFP_INSTRUCTION_FNMSCD + (1-dp) ]+= 1;//tianman
							_vfp_lock_analyze(rd, rn, rm, dp, ARM_VFP_INSTRUCTION_FNMSCD + (1-dp),&(s->tb->extra_tb_info) );
							_vfp_lock_release(ARM_VFP_INSTRUCTION_FNMSCD + (1-dp),&(s->tb->extra_tb_info));
							break;
						case 4: /* mul: fn * fm */
							s->tb->extra_tb_info.vfp_count[ARM_VFP_INSTRUCTION_FMULD + (1-dp) ]+= 1;//tianman
							_vfp_lock_analyze(rd, rn, rm, dp, ARM_VFP_INSTRUCTION_FMULD + (1-dp),&(s->tb->extra_tb_info) );
							_vfp_lock_release(ARM_VFP_INSTRUCTION_FMULD + (1-dp),&(s->tb->extra_tb_info));
							break;
						case 5: /* nmul: -(fn * fm) */
							s->tb->extra_tb_info.vfp_count[ARM_VFP_INSTRUCTION_FNMULD + (1-dp) ]+= 1;//tianman
							_vfp_lock_analyze(rd, rn, rm, dp, ARM_VFP_INSTRUCTION_FNMULD + (1-dp),&(s->tb->extra_tb_info) );
							_vfp_lock_release(ARM_VFP_INSTRUCTION_FNMULD + (1-dp),&(s->tb->extra_tb_info));
							break;
						case 6: /* add: fn + fm */
							s->tb->extra_tb_info.vfp_count[ARM_VFP_INSTRUCTION_FADDD + (1-dp) ]+= 1;//tianman
							_vfp_lock_analyze(rd, rn, rm, dp, ARM_VFP_INSTRUCTION_FADDD + (1-dp),&(s->tb->extra_tb_info) );
							_vfp_lock_release(ARM_VFP_INSTRUCTION_FADDD + (1-dp),&(s->tb->extra_tb_info));
							break;
						case 7: /* sub: fn - fm */
							s->tb->extra_tb_info.vfp_count[ARM_VFP_INSTRUCTION_FSUBD + (1-dp) ]+= 1;//tianman
							_vfp_lock_analyze(rd, rn, rm, dp, ARM_VFP_INSTRUCTION_FSUBD + (1-dp) ,&(s->tb->extra_tb_info));
							_vfp_lock_release(ARM_VFP_INSTRUCTION_FSUBD + (1-dp),&(s->tb->extra_tb_info));
							break;
						case 8: /* div: fn / fm */
							s->tb->extra_tb_info.vfp_count[ARM_VFP_INSTRUCTION_FDIVD + (1-dp) ]+= 1;//tianman
							// _vfp_lock_analyze(rd, rn, rm, dp, ARM_VFP_INSTRUCTION_FDIVD + (1-dp),&(s->tb->extra_tb_info) );
							_vfp_lock_release(ARM_VFP_INSTRUCTION_FDIVD + (1-dp),&(s->tb->extra_tb_info));
							break;
						case 14: /* fconst */

							n = (insn << 12) & 0x80000000;
							i = ((insn >> 12) & 0x70) | (insn & 0xf);
							if (dp) {
								if (i & 0x40)
									i |= 0x3f80;
								else
									i |= 0x4000;
								n |= i << 16;
							} else {
								if (i & 0x40)
									i |= 0x780;
								else
									i |= 0x800;
								n |= i << 19;
							}
							break;
						case 15: /* extension space */
							switch (rn) {
								case 0: /* cpy */
									s->tb->extra_tb_info.vfp_count[ARM_VFP_INSTRUCTION_FCPYD + (1-dp) ]+= 1;//tianman
									_vfp_lock_analyze(rd, rd, rm, dp, ARM_VFP_INSTRUCTION_FDIVD + (1-dp),&(s->tb->extra_tb_info) );
									_vfp_lock_release(ARM_VFP_INSTRUCTION_FCPYD + (1-dp),&(s->tb->extra_tb_info));
									/* no-op */
									break;
								case 1: /* abs */
									s->tb->extra_tb_info.vfp_count[ARM_VFP_INSTRUCTION_FABSD + (1-dp) ]+= 1;//tianman
									_vfp_lock_analyze(rd, rd, rm, dp, ARM_VFP_INSTRUCTION_FDIVD + (1-dp),&(s->tb->extra_tb_info) );
									_vfp_lock_release(ARM_VFP_INSTRUCTION_FABSD + (1-dp),&(s->tb->extra_tb_info));
									break;
								case 2: /* neg */
									s->tb->extra_tb_info.vfp_count[ARM_VFP_INSTRUCTION_FNEGD + (1-dp) ]+= 1;//tianman
									_vfp_lock_analyze(rd, rd, rm, dp, ARM_VFP_INSTRUCTION_FDIVD + (1-dp),&(s->tb->extra_tb_info) );
									_vfp_lock_release(ARM_VFP_INSTRUCTION_FNEGD + (1-dp),&(s->tb->extra_tb_info));
									break;
								case 3: /* sqrt */
									s->tb->extra_tb_info.vfp_count[ARM_VFP_INSTRUCTION_FSQRTD + (1-dp) ]+= 1;//tianman
									// _vfp_lock_analyze(rd, rd, rm, dp, ARM_VFP_INSTRUCTION_FDIVD + (1-dp),&(s->tb->extra_tb_info) );
									_vfp_lock_release(ARM_VFP_INSTRUCTION_FSQRTD + (1-dp),&(s->tb->extra_tb_info));
									break;
								case 8: /* cmp */
									s->tb->extra_tb_info.vfp_count[ARM_VFP_INSTRUCTION_FCMPD + (1-dp) ]+= 1;//tianman
									_vfp_lock_analyze(rd, rd, rm, dp, ARM_VFP_INSTRUCTION_FDIVD + (1-dp),&(s->tb->extra_tb_info) );
									_vfp_lock_release(ARM_VFP_INSTRUCTION_FCMPD + (1-dp),&(s->tb->extra_tb_info));
									break;
								case 9: /* cmpe */
									s->tb->extra_tb_info.vfp_count[ARM_VFP_INSTRUCTION_FCMPED + (1-dp) ]+= 1;//tianman
									_vfp_lock_analyze(rd, rd, rm, dp, ARM_VFP_INSTRUCTION_FDIVD + (1-dp),&(s->tb->extra_tb_info) );
									_vfp_lock_release(ARM_VFP_INSTRUCTION_FCMPED + (1-dp),&(s->tb->extra_tb_info));
									break;
								case 10: /* cmpz */
									s->tb->extra_tb_info.vfp_count[ARM_VFP_INSTRUCTION_FCMPZD + (1-dp) ]+= 1;//tianman
									_vfp_lock_analyze(rd, rd, rd, dp, ARM_VFP_INSTRUCTION_FDIVD + (1-dp),&(s->tb->extra_tb_info) );
									_vfp_lock_release(ARM_VFP_INSTRUCTION_FCMPZD + (1-dp),&(s->tb->extra_tb_info));
									break;
								case 11: /* cmpez */
									s->tb->extra_tb_info.vfp_count[ARM_VFP_INSTRUCTION_FCMPEZD + (1-dp) ]+= 1;//tianman
									_vfp_lock_analyze(rd, rd, rd, dp, ARM_VFP_INSTRUCTION_FDIVD + (1-dp),&(s->tb->extra_tb_info) );
									_vfp_lock_release(ARM_VFP_INSTRUCTION_FCMPEZD + (1-dp),&(s->tb->extra_tb_info));
									break;
								case 15: /* single<->double conversion */
									if (dp){
										s->tb->extra_tb_info.vfp_count[ARM_VFP_INSTRUCTION_FCVTSD]+= 1;//tianman
										_vfp_lock_release(ARM_VFP_INSTRUCTION_FCVTSD + (1-dp),&(s->tb->extra_tb_info));
									}
									else{
										s->tb->extra_tb_info.vfp_count[ARM_VFP_INSTRUCTION_FCVTDS]+= 1;//tianman
										_vfp_lock_release(ARM_VFP_INSTRUCTION_FCVTDS + (1-dp),&(s->tb->extra_tb_info));
									}
									break;
								case 16: /* fuito */
									s->tb->extra_tb_info.vfp_count[ARM_VFP_INSTRUCTION_FUITOD + (1-dp) ]+= 1;//tianman
									_vfp_lock_release(ARM_VFP_INSTRUCTION_FUITOD + (1-dp),&(s->tb->extra_tb_info));
									break;
								case 17: /* fsito */
									s->tb->extra_tb_info.vfp_count[ARM_VFP_INSTRUCTION_FSITOD + (1-dp) ]+= 1;//tianman
									_vfp_lock_release(ARM_VFP_INSTRUCTION_FSITOD + (1-dp),&(s->tb->extra_tb_info));
									break;
								case 20: /* fshto */
									break;
								case 21: /* fslto */
									break;
								case 22: /* fuhto */
									break;
								case 23: /* fulto */
									break;
								case 24: /* ftoui */
									s->tb->extra_tb_info.vfp_count[ARM_VFP_INSTRUCTION_FTOUID + (1-dp) ]+= 1;//tianman
									_vfp_lock_release(ARM_VFP_INSTRUCTION_FTOUID + (1-dp),&(s->tb->extra_tb_info));
									break;
								case 25: /* ftouiz */
									s->tb->extra_tb_info.vfp_count[ARM_VFP_INSTRUCTION_FTOUIZD + (1-dp) ]+= 1;//tianman
									_vfp_lock_release(ARM_VFP_INSTRUCTION_FTOUIZD + (1-dp),&(s->tb->extra_tb_info));
									break;
								case 26: /* ftosi */
									s->tb->extra_tb_info.vfp_count[ARM_VFP_INSTRUCTION_FTOSID + (1-dp) ]+= 1;//tianman
									_vfp_lock_release(ARM_VFP_INSTRUCTION_FTOSID + (1-dp),&(s->tb->extra_tb_info));
									break;
								case 27: /* ftosiz */
									s->tb->extra_tb_info.vfp_count[ARM_VFP_INSTRUCTION_FTOUSIZD + (1-dp) ]+= 1;//tianman
									_vfp_lock_release(ARM_VFP_INSTRUCTION_FTOUSIZD + (1-dp),&(s->tb->extra_tb_info));
									break;
								case 28: /* ftosh */
									break;
								case 29: /* ftosl */
									break;
								case 30: /* ftouh */
									break;
								case 31: /* ftoul */
									break;
								default: /* undefined */
									printf ("rn:%d\n", rn);
									return 1;
							}
							break;
						default: /* undefined */
							printf ("op:%d\n", op);
							return 1;
					}

					/* Write back the result.  */
					if (op == 15 && (rn >= 8 && rn <= 11))
					{}  /* Comparison, do nothing.  */
					else if (op == 15 && rn > 17)
					{}   /* Integer result.  */
					else if (op == 15 && rn == 15)
					{}    /* conversion */
					else
					{}
					/* break out of the loop if we have finished  */
					if (veclen == 0)
						break;

					if (op == 15 && delta_m == 0) {
						/* single source one-many */
						while (veclen--) {
							rd = ((rd + delta_d) & (bank_mask - 1))
								| (rd & bank_mask);
						}
						break;
					}
					/* Setup the next operands.  */
					veclen--;
					rd = ((rd + delta_d) & (bank_mask - 1))
						| (rd & bank_mask);

					if (op == 15) {
						/* One source operand.  */
						rm = ((rm + delta_m) & (bank_mask - 1))
							| (rm & bank_mask);
					} else {
						/* Two source operands.  */
						rn = ((rn + delta_d) & (bank_mask - 1))
							| (rn & bank_mask);
						if (delta_m) {
							rm = ((rm + delta_m) & (bank_mask - 1))
								| (rm & bank_mask);
						}
					}
				}
			}
			break;
		case 0xc:
		case 0xd:
			if (dp && (insn & 0x03e00000) == 0x00400000) {
				/* two-register transfer */
				rn = (insn >> 16) & 0xf;
				rd = (insn >> 12) & 0xf;
				if (dp) {
					VFP_DREG_M(rm, insn);
				} else {
					rm = VFP_SREG_M(insn);
				}

				if (insn & ARM_CP_RW_BIT) {
					/* vfp->arm */
					if (dp) {
						s->tb->extra_tb_info.vfp_count[ARM_VFP_INSTRUCTION_FMRRD]+= 1;//tianman
						_vfp_lock_release(ARM_VFP_INSTRUCTION_FMRRD,&(s->tb->extra_tb_info));
					} else {
						s->tb->extra_tb_info.vfp_count[ARM_VFP_INSTRUCTION_FMRRS]+= 1;//tianman
						_vfp_lock_release(ARM_VFP_INSTRUCTION_FMRRS,&(s->tb->extra_tb_info));
					}
				} else {
					/* arm->vfp */
					if (dp) {
						s->tb->extra_tb_info.vfp_count[ARM_VFP_INSTRUCTION_FMDRR]+= 1;//tianman
						_vfp_lock_release(ARM_VFP_INSTRUCTION_FMDRR,&(s->tb->extra_tb_info));
					} else {
						s->tb->extra_tb_info.vfp_count[ARM_VFP_INSTRUCTION_FMSRR]+= 1;//tianman
						_vfp_lock_release(ARM_VFP_INSTRUCTION_FMSRR,&(s->tb->extra_tb_info));
					}
				}
			} else {
				/* Load/store */
				rn = (insn >> 16) & 0xf;
				if (dp)
					VFP_DREG_D(rd, insn);
				else
					rd = VFP_SREG_D(insn);
				if (s->thumb && rn == 15) {
				} else {
				}
				if ((insn & 0x01200000) == 0x01000000) {
					/* Single load/store */
					offset = (insn & 0xff) << 2;
					if ((insn & (1 << 23)) == 0)
						offset = -offset;
					if (insn & (1 << 20)) {
						s->tb->extra_tb_info.vfp_count[ARM_VFP_INSTRUCTION_FLDD + (1-dp)]+= 1;//tianman
						_vfp_lock_release(ARM_VFP_INSTRUCTION_FLDD + (1-dp),&(s->tb->extra_tb_info));
					} else {
						s->tb->extra_tb_info.vfp_count[ARM_VFP_INSTRUCTION_FSTD + (1-dp)]+= 1;//tianman
						_vfp_lock_release(ARM_VFP_INSTRUCTION_FSTD + (1-dp),&(s->tb->extra_tb_info));
					}
				} else {
					/* load/store multiple */
					if (dp)
						n = (insn >> 1) & 0x7f;
					else
						n = insn & 0xff;

					if (insn & (1 << 24)) /* pre-decrement */
					{}

					if (dp)
						offset = 8;
					else
						offset = 4;

					for (i = 0; i < n; i++) {
						if (insn & ARM_CP_RW_BIT) {
							/* load */
							if(insn & 0x1){
								s->tb->extra_tb_info.vfp_count[ARM_VFP_INSTRUCTION_FLDMX]+= 1;//tianman
								_vfp_lock_release(ARM_VFP_INSTRUCTION_FLDMX,&(s->tb->extra_tb_info));
							}
							else{
								s->tb->extra_tb_info.vfp_count[ARM_VFP_INSTRUCTION_FLDMD + (1-dp)]+= 1;//tianman
								_vfp_lock_release(ARM_VFP_INSTRUCTION_FLDMD + (1-dp),&(s->tb->extra_tb_info));
							}
						} else {
							/* store */

							if(insn & 0x1){
								s->tb->extra_tb_info.vfp_count[ARM_VFP_INSTRUCTION_FSTMX]+= 1;//tianman
								_vfp_lock_release(ARM_VFP_INSTRUCTION_FSTMX,&(s->tb->extra_tb_info));
							}
							else{
								s->tb->extra_tb_info.vfp_count[ARM_VFP_INSTRUCTION_FSTMD + (1-dp)]+= 1;//tianman
								_vfp_lock_release(ARM_VFP_INSTRUCTION_FSTMD + (1-dp),&(s->tb->extra_tb_info));
							}

						}
					}
					if (insn & (1 << 21)) {
						/* writeback */
						if (insn & (1 << 24))
							offset = -offset * n;
						else if (dp && (insn & 1))
							offset = 4;
						else
							offset = 0;

						if (offset != 0){}
					}
				}
			}
			break;
		default:
			/* Should never happen.  */
			return 1;
	}
	return 0;
}
#endif

/*
 *  Implement by Tianman
 *	Simulate the EX stage in pipe line, without interlock.
 *  Accumulate the cycle count in each instruction counter in each TB.
 */
int analyze_arm_ticks(uint32_t insn, CPUState *env, DisasContext *s){

	unsigned int cond, val, op1, i, shift, rm, rs, rn, rd, sh;
	unsigned int instr_index = 0;//tianman
	
	//evo0209
	s->tb->extra_tb_info.insn_buf[s->tb->extra_tb_info.insn_buf_index] = insn;
	s->tb->extra_tb_info.insn_buf_index++;

	cond = insn >> 28;
	if (cond == 0xf){
		/* Unconditional instructions.  */
		if (((insn >> 25) & 7) == 1) {
			/* NEON Data processing.  */
			s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_NEON_DP]+=1;
			return;
		}
		if ((insn & 0x0f100000) == 0x04000000) {
			/* NEON load/store.  */
			s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_NEON_LS]+=1;
			return;
		}
		if ((insn & 0x0d70f000) == 0x0550f000)
		{
			s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_PLD]+=1;
			return; /* PLD */
		}
		else if ((insn & 0x0ffffdff) == 0x01010000) {
			s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_SETEND]+=1;//tianman
			/* setend */
			return;
		} else if ((insn & 0x0fffff00) == 0x057ff000) {
			switch ((insn >> 4) & 0xf) {
				case 1: /* clrex */
					s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_CLREX]+=1;//tianman
					return;
				case 4: /* dsb */
					s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_DSB]+=1;//tianman
				case 5: /* dmb */
					s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_DMB]+=1;//tianman
				case 6: /* isb */
					s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_ISB]+=1;//tianman
					/* We don't emulate caches so these are a no-op.  */
					return;
				default:
					goto illegal_op;
			}
		} else if ((insn & 0x0e5fffe0) == 0x084d0500) {
			/* srs */
			s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_SRS]+=1;//tianman
			return;
		} else if ((insn & 0x0e5fffe0) == 0x081d0a00) {
			/* rfe */
			s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_RFE]+=1;//tianman
			return;
		} else if ((insn & 0x0e000000) == 0x0a000000) {
			/* branch link and change to thumb (blx <offset>) */
			s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_BLX]+=1;//tianman
			return;
		} else if ((insn & 0x0e000f00) == 0x0c000100) {
			//LDC,STC?
			if (insn & (1 << 20)) {
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_LDC]+=1;//tianman
			}
			else{
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_STC]+=1;//tianman
			}
			return;
		} else if ((insn & 0x0fe00000) == 0x0c400000) {
			/* Coprocessor double register transfer.  */
			//MCRR, MRRC
			if (insn & (1 << 20)) {
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_MRRC]+=1;//tianman
			}
			else{
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_MCRR]+=1;//tianman
			}
			return;
		} else if ((insn & 0x0f000010) == 0x0e000010) {
			/* Additional coprocessor register transfer.  */
			//MCR,MRC
			if (insn & (1 << 20)) {
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_MRC]+=1;//tianman
			}
			else{
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_MCR]+=1;//tianman
			}
			return;
		} else if ((insn & 0x0ff10020) == 0x01000000) {
			/* cps (privileged) */
			s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_CPS]+=1;//tianman
			return;
		}
		goto illegal_op;
	}

	if ((insn & 0x0f900000) == 0x03000000) {
		if ((insn & (1 << 21)) == 0) {
			if ((insn & (1 << 22)) == 0) {
				/* MOVW */
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_MOVW]+=1;//tianman
			} else {
				/* MOVT */
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_MOVT]+=1;//tianman
			}
		} else {
			if (((insn >> 16) & 0xf) == 0) {
			} else {
				/* CPSR = immediate */
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_MSR]+=1;//tianman
			}
		}
	} else if ((insn & 0x0f900000) == 0x01000000
			&& (insn & 0x00000090) != 0x00000090) {
		/* miscellaneous instructions */
		op1 = (insn >> 21) & 3;
		sh = (insn >> 4) & 0xf;
		switch (sh) {
			case 0x0: /* move program status register */
				if (op1 & 1) {
					/* PSR = reg */
					s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_MSR]+=1;//tianman
				} else {
					/* reg = PSR */
					s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_MRS]+=1;//tianman
				}
				break;
			case 0x1:
				if (op1 == 1) {
					s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_BX]+=1;//tianman
					/* branch/exchange thumb (bx).  */
				} else if (op1 == 3) {
					/* clz */
					s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_CLZ]+=1;//tianman
				} else {
					goto illegal_op;
				}
				break;
			case 0x2:
				if (op1 == 1) {
					s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_BXJ]+=1;//tianman
				} else {
					goto illegal_op;
				}
				break;
			case 0x3:
				if (op1 != 1)
					goto illegal_op;
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_BLX]+=1;//tianman

				/* branch link/exchange thumb (blx) */
				break;
			case 0x5: /* saturating add/subtract */
				if (op1 & 2){
					if (op1 & 1)
						s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_QDSUB]+=1;//tianman
					else
						s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_QDADD]+=1;//tianman
				}
				if (op1 & 1){
					s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_QSUB]+=1;//tianman
				}
				else{
					s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_QADD]+=1;//tianman
				}
				break;
			case 7: /* bkpt */
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_BKPT]+=1;//tianman
				break;
			case 0x8: /* signed multiply */
			case 0xa:
			case 0xc:
			case 0xe:
				if (op1 == 1) {
					/* (32 * 16) >> 16 */
					if ((sh & 2) == 0) {
						s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_SMLAWY]+=1;//tianman
					}
					s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_SMULWY]+=1;//tianman
				} else {
					/* 16 * 16 */
					if(op1 == 3)
						s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_SMULXY]+=1;//tianman
					if (op1 == 2) {
						s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_SMLALXY]+=1;//tianman
					} else {
						if (op1 == 0) {
							s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_SMLAXY]+=1;//tianman
						}
					}
				}
				break;
			default:
				goto illegal_op;
		}
	} else if (((insn & 0x0e000000) == 0 &&
				(insn & 0x00000090) != 0x90) ||
			((insn & 0x0e000000) == (1 << 25))) {
#if 0
		int set_cc, logic_cc, shiftop;

		set_cc = (insn >> 20) & 1;
		logic_cc = table_logic_cc[op1] & set_cc;

		/* data processing instruction */
		if (insn & (1 << 25)) {
			/* immediate operand */
			val = insn & 0xff;
			shift = ((insn >> 8) & 0xf) * 2;
			if (shift) {
				val = (val >> shift) | (val << (32 - shift));
			}
			tmp2 = new_tmp();
			tcg_gen_movi_i32(tmp2, val);
			if (logic_cc && shift) {
				gen_set_CF_bit31(tmp2);
			}
		} else {
			/* register */
			rm = (insn) & 0xf;
			tmp2 = load_reg(s, rm);
			shiftop = (insn >> 5) & 3;
			if (!(insn & (1 << 4))) {
				shift = (insn >> 7) & 0x1f;
				gen_arm_shift_im(tmp2, shiftop, shift, logic_cc);
			} else {
				rs = (insn >> 8) & 0xf;
				tmp = load_reg(s, rs);
				gen_arm_shift_reg(tmp2, shiftop, tmp, logic_cc);
			}
		}
		if (op1 != 0x0f && op1 != 0x0d) {
			rn = (insn >> 16) & 0xf;
			tmp = load_reg(s, rn);
		} else {
			TCGV_UNUSED(tmp);
		}
#endif        
		op1 = (insn >> 21) & 0xf;
		rd = (insn >> 12) & 0xf;
		switch(op1) {
			case 0x00:
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_AND]+=1;//tianman
				break;
			case 0x01:
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_EOR]+=1;//tianman
				break;
			case 0x02:
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_SUB]+=1;//tianman
				break;
			case 0x03:
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_RSB]+=1;//tianman
				break;
			case 0x04:
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_ADD]+=1;//tianman
				break;
			case 0x05:
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_ADC]+=1;//tianman
				break;
			case 0x06:
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_SBC]+=1;//tianman
				break;
			case 0x07:
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_RSC]+=1;//tianman
				break;
			case 0x08:
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_TST]+=1;//tianman
				break;
			case 0x09:
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_TEQ]+=1;//tianman
				break;
			case 0x0a:
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_CMP]+=1;//tianman
				break;
			case 0x0b:
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_CMN]+=1;//tianman
				break;
			case 0x0c:
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_ORR]+=1;//tianman
				break;
			case 0x0d:
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_MOV]+=1;//tianman
				break;
			case 0x0e:
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_BIC]+=1;//tianman
				break;
			default:
			case 0x0f:
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_MVN]+=1;//tianman
				break;
		}
	} else {
		/* other instructions */
		op1 = (insn >> 24) & 0xf;
		switch(op1) {
			case 0x0:
			case 0x1:
				/* multiplies, extra load/stores */
				sh = (insn >> 5) & 3;
				if (sh == 0) {
					if (op1 == 0x0) {
						op1 = (insn >> 20) & 0xf;
						switch (op1) {
							case 0: case 1: case 2: case 3: case 6:
								/* 32 bit mul */
								instr_index = ARM_INSTRUCTION_MUL;//tianman
								if (insn & (1 << 22)) {
									/* Subtract (mls) */
								} else if (insn & (1 << 21)) {
									/* Add */
									instr_index = ARM_INSTRUCTION_MLA;//tianman
								}
								if (insn & (1 << 20)){
									instr_index++;//tianman
								}
								s->tb->extra_tb_info.arm_count[instr_index]+=1;//tianman
								break;
							default:
								/* 64 bit mul */
								if (insn & (1 << 22)){
									instr_index = ARM_INSTRUCTION_SMULL;//tianman
								}
								else{
									instr_index = ARM_INSTRUCTION_UMULL;//tianman
								}
								if (insn & (1 << 21)){ /* mult accumulate */
									if (insn & (1 << 22)){
										instr_index = ARM_INSTRUCTION_SMLAL;//tianman
									}
									else{
										instr_index = ARM_INSTRUCTION_UMLAL;//tianman
									}
								}
								if (!(insn & (1 << 23))) { /* double accumulate */
								}
								if (insn & (1 << 20)){
									instr_index++;
								}
								s->tb->extra_tb_info.arm_count[instr_index]+=1;//tianman
								break;
						}
					} else {
						if (insn & (1 << 23)) {
							/* load/store exclusive */
							op1 = (insn >> 21) & 0x3;
							if (insn & (1 << 20)) {
								switch (op1) {
									s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_LDREX]+=1;//tianman
									case 0: /* ldrex */
									break;
									case 1: /* ldrexd */
									break;
									case 2: /* ldrexb */
									break;
									case 3: /* ldrexh */
									break;
									default:
									abort();
								}
							} else {
								s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_STREX]+=1;//tianman
								switch (op1) {
									case 0:  /*  strex */
										break;
									case 1: /*  strexd */
										break;
									case 2: /*  strexb */
										break;
									case 3: /* strexh */
										break;
									default:
										abort();
								}
							}
						} else {
							/* SWP instruction */

							/* ??? This is not really atomic.  However we know
							   we never have multiple CPUs running in parallel,
							   so it is good enough.  */
							if (insn & (1 << 22)) {
								s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_SWPB]+=1;//tianman
							} else {
								s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_SWP]+=1;//tianman
							}
						}
					}
				} else {
					/* Misc load/store */
					if (insn & (1 << 24)){
						/* Misc load/store */
					}
					if (insn & (1 << 20)) {
						/* load */
						switch(sh) {
							case 1:
								s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_LDRH]+=1;//tianman
								break;
							case 2:
								s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_LDRSB]+=1;//tianman
								break;
							default:
								s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_LDRSH]+=1;//tianman
							case 3:
								break;
						}
					} else if (sh & 2) {
						/* doubleword */
						if (sh & 1) {
							/* store */
							s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_STRD]+=1;//tianman
						} else {
							/* load */
							s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_LDRD]+=1;//tianman
						}
					} else {
						/* store */
						s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_STRH]+=1;//tianman
					}
					/* Perform base writeback before the loaded value to
					   ensure correct behavior with overlapping index registers.
					   ldrd with base writeback is is undefined if the
					   destination and index registers overlap.  */
					if (!(insn & (1 << 24))) {
					} else if (insn & (1 << 21)) {
					} else {
					}
				}
				break;
			case 0x4:
			case 0x5:
				goto do_ldst_tianman;
			case 0x6:
			case 0x7:
				if (insn & (1 << 4)) {
					/* Armv6 Media instructions.  */
					switch ((insn >> 23) & 3) {
						case 0: /* Parallel add/subtract.  */
							break;
						case 1:
							if ((insn & 0x00700020) == 0) {
								/* Halfword pack.  */
								shift = (insn >> 7) & 0x1f;
								if (insn & (1 << 6)) {
									/* pkhtb */
									s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_PKHTB]+=1;//tianman
									if (shift == 0)
										shift = 31;
								} else {
									/* pkhbt */
									s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_PKHBT]+=1;//tianman
								}
							} else if ((insn & 0x00200020) == 0x00200000) {
								/* [us]sat */
								sh = (insn >> 16) & 0x1f;
								if (sh != 0) {
									if (insn & (1 << 22)){
										s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_USAT]+=1;//tianman
									}
									else{
										s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_SSAT]+=1;//tianman
									}
								}
							} else if ((insn & 0x00300fe0) == 0x00200f20) {
								/* [us]sat16 */
								sh = (insn >> 16) & 0x1f;
								if (sh != 0) {
									if (insn & (1 << 22)){
										s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_USAT16]+=1;//tianman
									}
									else{
										s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_SSAT16]+=1;//tianman
									}
								}
							} else if ((insn & 0x00700fe0) == 0x00000fa0) {
								/* Select bytes.  */
								s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_SEL]+=1;//tianman
							} else if ((insn & 0x000003e0) == 0x00000060) {
								//shift = (insn >> 10) & 3;
								/* ??? In many cases it's not neccessary to do a
								   rotate, a shift is sufficient.  */
								//                        if (shift != 0)
								//                            tcg_gen_rotri_i32(tmp, tmp, shift * 8);
								op1 = (insn >> 20) & 7;
								switch (op1) {
									case 0:
										instr_index = ARM_INSTRUCTION_SXTB16;//tianman
										break;
									case 2:
										instr_index = ARM_INSTRUCTION_SXTB;//tianman
										break;
									case 3:
										instr_index = ARM_INSTRUCTION_SXTH;//tianman
										break;
									case 4:
										instr_index = ARM_INSTRUCTION_UXTB16;//tianman
										break;
									case 6:
										instr_index = ARM_INSTRUCTION_UXTB;//tianman
										break;
									case 7:
										instr_index = ARM_INSTRUCTION_UXTH;//tianman
										break;
									default: goto illegal_op;
								}
								if (rn != 15) {
									instr_index -= 3;//tianman
								}
								s->tb->extra_tb_info.arm_count[instr_index]+=1;//tianman
							} else if ((insn & 0x003f0f60) == 0x003f0f20) {
								/* rev */
								if (insn & (1 << 22)) {
									if (insn & (1 << 7)) {
									} else {
									}
								} else {
									if (insn & (1 << 7)){
										s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_REV16]+=1;//tianman
									}
									else{
										s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_REV]+=1;//tianman
									}
								}
							} else {
								goto illegal_op;
							}
							break;
						case 2: /* Multiplies (Type 3).  */
							if (insn & (1 << 20)) {
								/* Signed multiply most significant [accumulate].  */
								if (rd != 15) {
									if (insn & (1 << 6)) {
										s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_SMMLS]+=1;//tianman
									} else {
										s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_SMMLA]+=1;//tianman
									}
								}
								else{
									s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_SMMUL]+=1;//tianman
								}
							} else {
								/* This addition cannot overflow.  */
								if (insn & (1 << 6)) {
									instr_index = 1;//tianman
								} else {
									instr_index = 0;//tianman
								}
								if (insn & (1 << 22)) {
									/* smlald, smlsld */
									instr_index += ARM_INSTRUCTION_SMLALD;//tianman
									s->tb->extra_tb_info.arm_count[instr_index]+=1;//tianman
								} else {
									/* smuad, smusd, smlad, smlsd */
									if (rd != 15){
										instr_index += 2;//tianman
									}
									instr_index += ARM_INSTRUCTION_SMUAD;//tianman
									s->tb->extra_tb_info.arm_count[instr_index]+=1;//tianman
								}
							}
							break;
						case 3:
							op1 = ((insn >> 17) & 0x38) | ((insn >> 5) & 7);
							switch (op1) {
								case 0: /* Unsigned sum of absolute differences.  */
									if (rd != 15) {
										s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_USADA8]+=1;//tianman
									}
									s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_USAD8]+=1;//tianman
									break;
								case 0x20: case 0x24: case 0x28: case 0x2c:
									/* Bitfield insert/clear.  */
									break;
								case 0x12: case 0x16: case 0x1a: case 0x1e: /* sbfx */
								case 0x32: case 0x36: case 0x3a: case 0x3e: /* ubfx */
									break;
								default:
									goto illegal_op;
							}
							break;
					}
					break;
				}
do_ldst_tianman:
				/* Check for undefined extension instructions
				 * per the ARM Bible IE:
				 * xxxx 0111 1111 xxxx  xxxx xxxx 1111 xxxx
				 */
				sh = (0xf << 20) | (0xf << 4);
				if (op1 == 0x7 && ((insn & sh) == sh))
				{
					goto illegal_op;
				}
				/* load/store byte/word */
				rn = (insn >> 16) & 0xf;
				rd = (insn >> 12) & 0xf;
				i = (IS_USER(s) || (insn & 0x01200000) == 0x00200000);
				if (insn & (1 << 20)) {
					/* load */
					if (insn & (1 << 22)) {
						s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_LDRB]+=1;//tianman
					} else {
						s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_LDR]+=1;//tianman
					}
				} else {
					/* store */
					if (insn & (1 << 22))
					{
						s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_STRB]+=1;//tianman
					}
					else
					{
						s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_STR]+=1;//tianman
					}
				}
//				if (insn & (1 << 20)) {
//					/* Complete the load.  */
//				}
				break;
			case 0x08:
			case 0x09:
				{
					int j, n, user, loaded_base;
					TCGv loaded_var;
					/* load/store multiple words */
					/* XXX: store correct base if write back */
					switch (insn & 0x00500000 >> 20) {//tianman
						case 0x0:
							s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_STM1]+=1;//tianman
							break;
						case 0x1:
							s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_LDM1]+=1;//tianman
							break;
						case 0x4:
							s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_STM2]+=1;//tianman
							break;
						case 0x5:
							if (insn & (1 << 15))
								s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_LDM3]+=1;//tianman
							else
								s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_LDM2]+=1;//tianman
							break;
					}
#if 0
					user = 0;
					if (insn & (1 << 22)) {
						if (IS_USER(s))
							goto illegal_op; /* only usable in supervisor mode */

						if ((insn & (1 << 15)) == 0)
							user = 1;
					}
					rn = (insn >> 16) & 0xf;
					addr = load_reg(s, rn);

					/* compute total size */
					loaded_base = 0;
					TCGV_UNUSED(loaded_var);
					n = 0;
					for(i=0;i<16;i++) {
						if (insn & (1 << i))
							n++;
					}
					/* XXX: test invalid n == 0 case ? */
					if (insn & (1 << 23)) {
						if (insn & (1 << 24)) {
							/* pre increment */
							tcg_gen_addi_i32(addr, addr, 4);
						} else {
							/* post increment */
						}
					} else {
						if (insn & (1 << 24)) {
							/* pre decrement */
							tcg_gen_addi_i32(addr, addr, -(n * 4));
						} else {
							/* post decrement */
							if (n != 1)
								tcg_gen_addi_i32(addr, addr, -((n - 1) * 4));
						}
					}
					if(insn & (1 << 20)) {
						/* paslab : load instructions counter += 1 */
						s->tb->extra_tb_info.load_count += 1;
					} else {
						/* paslab : store instructions counter += 1 */
						s->tb->extra_tb_info.store_count += 1;
					}
					j = 0;
					for(i=0;i<16;i++) {
						if (insn & (1 << i)) {
							if (insn & (1 << 20)) {
								/* load */
								if (i == 15) {
								} else if (user) {
								} else if (i == rn) {
								} else {
								}
							} else {
								/* store */
								if (i == 15) {
									/* special case: r15 = PC + 8 */
								} else if (user) {
								} else {
								}
							}
							j++;
							/* no need to add after the last transfer */
						}
					}
					if (insn & (1 << 21)) {
						/* write back */
						if (insn & (1 << 23)) {
							if (insn & (1 << 24)) {
								/* pre increment */
							} else {
								/* post increment */
							}
						} else {
							if (insn & (1 << 24)) {
								/* pre decrement */
								if (n != 1)
							} else {
								/* post decrement */
							}
						}
					} else {
					}
					if ((insn & (1 << 22)) && !user) {
						/* Restore CPSR from SPSR.  */
					}
#endif
				}
				break;
			case 0xa:
			case 0xb:
				{
					/* paslab : support event 0x05 (B & BL count) */
					s->tb->extra_tb_info.branch_count += 1;
					int32_t offset;

					/* branch (and link) */
					if (insn & (1 << 24)) {
						s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_B]+=1;//tianman
					}
					else
						s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_B]+=1;//tianman

				}
				break;
			case 0xc:
			case 0xd:
			case 0xe:
				/* Coprocessor.  */
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_COPROCESSOR]+=1;//tianman
				break;
			case 0xf:
				/* swi */
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_SWI]+=1;//tianman
				break;
			default:
illegal_op:
				break;
		}
	}
}

/* Implement by evo0209
 * It's almost the same as analyze_arm_ticks, but return the insn latency.
 * It aims to calculate the ticks in each TB, instead of in helper function
 */
uint32_t get_arm_ticks(uint32_t insn, CPUState *env, DisasContext *s){

	unsigned int cond, val, op1, i, shift, rm, rs, rn, rd, sh;
	unsigned int instr_index = 0;//tianman
	
	//evo0209
	s->tb->extra_tb_info.insn_buf[s->tb->extra_tb_info.insn_buf_index] = insn;
	s->tb->extra_tb_info.insn_buf_index++;

	cond = insn >> 28;
	if (cond == 0xf){
		/* Unconditional instructions.  */
		if (((insn >> 25) & 7) == 1) {
			/* NEON Data processing.  */
			s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_NEON_DP]+=1;
			return arm_instr_time[ARM_INSTRUCTION_NEON_DP];
		}
		if ((insn & 0x0f100000) == 0x04000000) {
			/* NEON load/store.  */
			s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_NEON_LS]+=1;
			return arm_instr_time[ARM_INSTRUCTION_NEON_LS];
		}
		if ((insn & 0x0d70f000) == 0x0550f000)
		{
			s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_PLD]+=1;
			return arm_instr_time[ARM_INSTRUCTION_PLD]; /* PLD */
		}
		else if ((insn & 0x0ffffdff) == 0x01010000) {
			s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_SETEND]+=1;//tianman
			/* setend */
			return arm_instr_time[ARM_INSTRUCTION_SETEND];
		} else if ((insn & 0x0fffff00) == 0x057ff000) {
			switch ((insn >> 4) & 0xf) {
				case 1: /* clrex */
					s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_CLREX]+=1;//tianman
					return arm_instr_time[ARM_INSTRUCTION_CLREX];
				case 4: /* dsb */
					s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_DSB]+=1;//tianman
					return arm_instr_time[ARM_INSTRUCTION_DSB];
				case 5: /* dmb */
					s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_DMB]+=1;//tianman
					return arm_instr_time[ARM_INSTRUCTION_DMB];
				case 6: /* isb */
					s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_ISB]+=1;//tianman
					/* We don't emulate caches so these are a no-op.  */
					return arm_instr_time[ARM_INSTRUCTION_ISB];
				default:
					goto illegal_op;
			}
		} else if ((insn & 0x0e5fffe0) == 0x084d0500) {
			/* srs */
			s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_SRS]+=1;//tianman
			return arm_instr_time[ARM_INSTRUCTION_SRS];
		} else if ((insn & 0x0e5fffe0) == 0x081d0a00) {
			/* rfe */
			s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_RFE]+=1;//tianman
			return arm_instr_time[ARM_INSTRUCTION_RFE];
		} else if ((insn & 0x0e000000) == 0x0a000000) {
			/* branch link and change to thumb (blx <offset>) */
			s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_BLX]+=1;//tianman
			return arm_instr_time[ARM_INSTRUCTION_BLX];
		} else if ((insn & 0x0e000f00) == 0x0c000100) {
			//LDC,STC?
			if (insn & (1 << 20)) {
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_LDC]+=1;//tianman
				return arm_instr_time[ARM_INSTRUCTION_LDC];
			}
			else{
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_STC]+=1;//tianman
				return arm_instr_time[ARM_INSTRUCTION_STC];
			}
			//return;
		} else if ((insn & 0x0fe00000) == 0x0c400000) {
			/* Coprocessor double register transfer.  */
			//MCRR, MRRC
			if (insn & (1 << 20)) {
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_MRRC]+=1;//tianman
				return arm_instr_time[ARM_INSTRUCTION_MRRC];
			}
			else{
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_MCRR]+=1;//tianman
				return arm_instr_time[ARM_INSTRUCTION_MCRR];
			}
			//return;
		} else if ((insn & 0x0f000010) == 0x0e000010) {
			/* Additional coprocessor register transfer.  */
			//MCR,MRC
			if (insn & (1 << 20)) {
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_MRC]+=1;//tianman
				return arm_instr_time[ARM_INSTRUCTION_MRC];
			}
			else{
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_MCR]+=1;//tianman
				return arm_instr_time[ARM_INSTRUCTION_MCR];
			}
			//return;
		} else if ((insn & 0x0ff10020) == 0x01000000) {
			/* cps (privileged) */
			s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_CPS]+=1;//tianman
			return arm_instr_time[ARM_INSTRUCTION_CPS];
		}
		goto illegal_op;
	}

	if ((insn & 0x0f900000) == 0x03000000) {
		if ((insn & (1 << 21)) == 0) {
			if ((insn & (1 << 22)) == 0) {
				/* MOVW */
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_MOVW]+=1;//tianman
				return arm_instr_time[ARM_INSTRUCTION_MOVW];
			} else {
				/* MOVT */
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_MOVT]+=1;//tianman
				return arm_instr_time[ARM_INSTRUCTION_MOVT];
			}
		} else {
			if (((insn >> 16) & 0xf) == 0) {
			} else {
				/* CPSR = immediate */
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_MSR]+=1;//tianman
				return arm_instr_time[ARM_INSTRUCTION_MSR];
			}
		}
	} else if ((insn & 0x0f900000) == 0x01000000
			&& (insn & 0x00000090) != 0x00000090) {
		/* miscellaneous instructions */
		op1 = (insn >> 21) & 3;
		sh = (insn >> 4) & 0xf;
		switch (sh) {
			case 0x0: /* move program status register */
				if (op1 & 1) {
					/* PSR = reg */
					s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_MSR]+=1;//tianman
					return arm_instr_time[ARM_INSTRUCTION_MSR];
				} else {
					/* reg = PSR */
					s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_MRS]+=1;//tianman
					return arm_instr_time[ARM_INSTRUCTION_MRS];
				}
				break;
			case 0x1:
				if (op1 == 1) {
					s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_BX]+=1;//tianman
					return arm_instr_time[ARM_INSTRUCTION_BX];
					/* branch/exchange thumb (bx).  */
				} else if (op1 == 3) {
					/* clz */
					s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_CLZ]+=1;//tianman
					return arm_instr_time[ARM_INSTRUCTION_CLZ];
				} else {
					goto illegal_op;
				}
				break;
			case 0x2:
				if (op1 == 1) {
					s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_BXJ]+=1;//tianman
					return arm_instr_time[ARM_INSTRUCTION_BXJ];
				} else {
					goto illegal_op;
				}
				break;
			case 0x3:
				if (op1 != 1)
					goto illegal_op;
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_BLX]+=1;//tianman
				return arm_instr_time[ARM_INSTRUCTION_BLX];

				/* branch link/exchange thumb (blx) */
				break;
			case 0x5: /* saturating add/subtract */
				if (op1 & 2){
					if (op1 & 1){
						s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_QDSUB]+=1;//tianman
						return arm_instr_time[ARM_INSTRUCTION_QDSUB];
					}
					else{
						s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_QDADD]+=1;//tianman
						return arm_instr_time[ARM_INSTRUCTION_QADD];
					}
				}
				if (op1 & 1){
					s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_QSUB]+=1;//tianman
					return arm_instr_time[ARM_INSTRUCTION_QSUB];
				}
				else{
					s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_QADD]+=1;//tianman
					return arm_instr_time[ARM_INSTRUCTION_QADD];
				}
				break;
			case 7: /* bkpt */
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_BKPT]+=1;//tianman
				return arm_instr_time[ARM_INSTRUCTION_BKPT];
				break;
			case 0x8: /* signed multiply */
			case 0xa:
			case 0xc:
			case 0xe:
				if (op1 == 1) {
					/* (32 * 16) >> 16 */
					if ((sh & 2) == 0) {
						s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_SMLAWY]+=1;//tianman
						return arm_instr_time[ARM_INSTRUCTION_SMLAWY];
					}
					s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_SMULWY]+=1;//tianman
					return arm_instr_time[ARM_INSTRUCTION_SMULWY];
				} else {
					/* 16 * 16 */
					if(op1 == 3) {
						s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_SMULXY]+=1;//tianman
						return arm_instr_time[ARM_INSTRUCTION_SMULXY];
					}
					else if (op1 == 2) {
						s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_SMLALXY]+=1;//tianman
						return arm_instr_time[ARM_INSTRUCTION_SMLALXY];
					} else {
						if (op1 == 0) {
							s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_SMLAXY]+=1;//tianman
							return arm_instr_time[ARM_INSTRUCTION_SMLAXY];
						}
					}
				}
				break;
			default:
				goto illegal_op;
		}
	} else if (((insn & 0x0e000000) == 0 &&
				(insn & 0x00000090) != 0x90) ||
			((insn & 0x0e000000) == (1 << 25))) {
#if 0
		int set_cc, logic_cc, shiftop;

		set_cc = (insn >> 20) & 1;
		logic_cc = table_logic_cc[op1] & set_cc;

		/* data processing instruction */
		if (insn & (1 << 25)) {
			/* immediate operand */
			val = insn & 0xff;
			shift = ((insn >> 8) & 0xf) * 2;
			if (shift) {
				val = (val >> shift) | (val << (32 - shift));
			}
			tmp2 = new_tmp();
			tcg_gen_movi_i32(tmp2, val);
			if (logic_cc && shift) {
				gen_set_CF_bit31(tmp2);
			}
		} else {
			/* register */
			rm = (insn) & 0xf;
			tmp2 = load_reg(s, rm);
			shiftop = (insn >> 5) & 3;
			if (!(insn & (1 << 4))) {
				shift = (insn >> 7) & 0x1f;
				gen_arm_shift_im(tmp2, shiftop, shift, logic_cc);
			} else {
				rs = (insn >> 8) & 0xf;
				tmp = load_reg(s, rs);
				gen_arm_shift_reg(tmp2, shiftop, tmp, logic_cc);
			}
		}
		if (op1 != 0x0f && op1 != 0x0d) {
			rn = (insn >> 16) & 0xf;
			tmp = load_reg(s, rn);
		} else {
			TCGV_UNUSED(tmp);
		}
#endif        
		op1 = (insn >> 21) & 0xf;
		rd = (insn >> 12) & 0xf;
		switch(op1) {
			case 0x00:
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_AND]+=1;//tianman
				return arm_instr_time[ARM_INSTRUCTION_AND];
				break;
			case 0x01:
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_EOR]+=1;//tianman
				return arm_instr_time[ARM_INSTRUCTION_EOR];
				break;
			case 0x02:
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_SUB]+=1;//tianman
				return arm_instr_time[ARM_INSTRUCTION_SUB];
				break;
			case 0x03:
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_RSB]+=1;//tianman
				return arm_instr_time[ARM_INSTRUCTION_RSB];
				break;
			case 0x04:
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_ADD]+=1;//tianman
				return arm_instr_time[ARM_INSTRUCTION_ADD];
				break;
			case 0x05:
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_ADC]+=1;//tianman
				return arm_instr_time[ARM_INSTRUCTION_ADC];
				break;
			case 0x06:
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_SBC]+=1;//tianman
				return arm_instr_time[ARM_INSTRUCTION_SBC];
				break;
			case 0x07:
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_RSC]+=1;//tianman
				return arm_instr_time[ARM_INSTRUCTION_RSC];
				break;
			case 0x08:
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_TST]+=1;//tianman
				return arm_instr_time[ARM_INSTRUCTION_TST];
				break;
			case 0x09:
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_TEQ]+=1;//tianman
				return arm_instr_time[ARM_INSTRUCTION_TEQ];
				break;
			case 0x0a:
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_CMP]+=1;//tianman
				return arm_instr_time[ARM_INSTRUCTION_CMP];
				break;
			case 0x0b:
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_CMN]+=1;//tianman
				return arm_instr_time[ARM_INSTRUCTION_CMN];
				break;
			case 0x0c:
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_ORR]+=1;//tianman
				return arm_instr_time[ARM_INSTRUCTION_ORR];
				break;
			case 0x0d:
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_MOV]+=1;//tianman
				return arm_instr_time[ARM_INSTRUCTION_MOV];
				break;
			case 0x0e:
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_BIC]+=1;//tianman
				return arm_instr_time[ARM_INSTRUCTION_BIC];
				break;
			default:
			case 0x0f:
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_MVN]+=1;//tianman
				return arm_instr_time[ARM_INSTRUCTION_MVN];
				break;
		}
	} else {
		/* other instructions */
		op1 = (insn >> 24) & 0xf;
		switch(op1) {
			case 0x0:
			case 0x1:
				/* multiplies, extra load/stores */
				sh = (insn >> 5) & 3;
				if (sh == 0) {
					if (op1 == 0x0) {
						op1 = (insn >> 20) & 0xf;
						switch (op1) {
							case 0: case 1: case 2: case 3: case 6:
								/* 32 bit mul */
								instr_index = ARM_INSTRUCTION_MUL;//tianman
								if (insn & (1 << 22)) {
									/* Subtract (mls) */
								} else if (insn & (1 << 21)) {
									/* Add */
									instr_index = ARM_INSTRUCTION_MLA;//tianman
								}
								if (insn & (1 << 20)){
									instr_index++;//tianman
								}
								s->tb->extra_tb_info.arm_count[instr_index]+=1;//tianman
								break;
							default:
								/* 64 bit mul */
								if (insn & (1 << 22)){
									instr_index = ARM_INSTRUCTION_SMULL;//tianman
								}
								else{
									instr_index = ARM_INSTRUCTION_UMULL;//tianman
								}
								if (insn & (1 << 21)){ /* mult accumulate */
									if (insn & (1 << 22)){
										instr_index = ARM_INSTRUCTION_SMLAL;//tianman
									}
									else{
										instr_index = ARM_INSTRUCTION_UMLAL;//tianman
									}
								}
								if (!(insn & (1 << 23))) { /* double accumulate */
								}
								if (insn & (1 << 20)){
									instr_index++;
								}
								s->tb->extra_tb_info.arm_count[instr_index]+=1;//tianman
								return arm_instr_time[instr_index];
								break;
						}
					} else {
						if (insn & (1 << 23)) {
							/* load/store exclusive */
							op1 = (insn >> 21) & 0x3;
							if (insn & (1 << 20)) {
								switch (op1) {
									s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_LDREX]+=1;//tianman
									case 0: /* ldrex */
									break;
									case 1: /* ldrexd */
									break;
									case 2: /* ldrexb */
									break;
									case 3: /* ldrexh */
									break;
									default:
									abort();
								}
								return arm_instr_time[ARM_INSTRUCTION_LDREX];
							} else {
								s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_STREX]+=1;//tianman
								switch (op1) {
									case 0:  /*  strex */
										break;
									case 1: /*  strexd */
										break;
									case 2: /*  strexb */
										break;
									case 3: /* strexh */
										break;
									default:
										abort();
								}
								return arm_instr_time[ARM_INSTRUCTION_STREX];
							}
						} else {
							/* SWP instruction */

							/* ??? This is not really atomic.  However we know
							   we never have multiple CPUs running in parallel,
							   so it is good enough.  */
							if (insn & (1 << 22)) {
								s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_SWPB]+=1;//tianman
								return arm_instr_time[ARM_INSTRUCTION_SWPB];
							} else {
								s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_SWP]+=1;//tianman
								return arm_instr_time[ARM_INSTRUCTION_SWP];
							}
						}
					}
				} else {
					/* Misc load/store */
					if (insn & (1 << 24)){
						/* Misc load/store */
					}
					if (insn & (1 << 20)) {
						/* load */
						switch(sh) {
							case 1:
								s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_LDRH]+=1;//tianman
								return arm_instr_time[ARM_INSTRUCTION_LDRH];
								break;
							case 2:
								s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_LDRSB]+=1;//tianman
								return arm_instr_time[ARM_INSTRUCTION_LDRSB];
								break;
							default:
								s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_LDRSH]+=1;//tianman
								return arm_instr_time[ARM_INSTRUCTION_LDRSH];
							case 3:
								break;
						}
					} else if (sh & 2) {
						/* doubleword */
						if (sh & 1) {
							/* store */
							s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_STRD]+=1;//tianman
							return arm_instr_time[ARM_INSTRUCTION_STRD];
						} else {
							/* load */
							s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_LDRD]+=1;//tianman
							return arm_instr_time[ARM_INSTRUCTION_LDRD];
						}
					} else {
						/* store */
						s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_STRH]+=1;//tianman
						return arm_instr_time[ARM_INSTRUCTION_STRH];
					}
					/* Perform base writeback before the loaded value to
					   ensure correct behavior with overlapping index registers.
					   ldrd with base writeback is is undefined if the
					   destination and index registers overlap.  */
					if (!(insn & (1 << 24))) {
					} else if (insn & (1 << 21)) {
					} else {
					}
				}
				break;
			case 0x4:
			case 0x5:
				goto do_ldst_tianman;
			case 0x6:
			case 0x7:
				if (insn & (1 << 4)) {
					/* Armv6 Media instructions.  */
					switch ((insn >> 23) & 3) {
						case 0: /* Parallel add/subtract.  */
							break;
						case 1:
							if ((insn & 0x00700020) == 0) {
								/* Halfword pack.  */
								shift = (insn >> 7) & 0x1f;
								if (insn & (1 << 6)) {
									/* pkhtb */
									s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_PKHTB]+=1;//tianman
									if (shift == 0)
										shift = 31;
									return arm_instr_time[ARM_INSTRUCTION_PKHTB];
								} else {
									/* pkhbt */
									s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_PKHBT]+=1;//tianman
									return arm_instr_time[ARM_INSTRUCTION_PKHBT];
								}
							} else if ((insn & 0x00200020) == 0x00200000) {
								/* [us]sat */
								sh = (insn >> 16) & 0x1f;
								if (sh != 0) {
									if (insn & (1 << 22)){
										s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_USAT]+=1;//tianman
										return arm_instr_time[ARM_INSTRUCTION_USAT];
									}
									else{
										s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_SSAT]+=1;//tianman
										return arm_instr_time[ARM_INSTRUCTION_SSAT];
									}
								}
							} else if ((insn & 0x00300fe0) == 0x00200f20) {
								/* [us]sat16 */
								sh = (insn >> 16) & 0x1f;
								if (sh != 0) {
									if (insn & (1 << 22)){
										s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_USAT16]+=1;//tianman
										return arm_instr_time[ARM_INSTRUCTION_USAT16];
									}
									else{
										s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_SSAT16]+=1;//tianman
										return arm_instr_time[ARM_INSTRUCTION_SSAT16];
									}
								}
							} else if ((insn & 0x00700fe0) == 0x00000fa0) {
								/* Select bytes.  */
								s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_SEL]+=1;//tianman
								return arm_instr_time[ARM_INSTRUCTION_SEL];
							} else if ((insn & 0x000003e0) == 0x00000060) {
								//shift = (insn >> 10) & 3;
								/* ??? In many cases it's not neccessary to do a
								   rotate, a shift is sufficient.  */
								//                        if (shift != 0)
								//                            tcg_gen_rotri_i32(tmp, tmp, shift * 8);
								op1 = (insn >> 20) & 7;
								switch (op1) {
									case 0:
										instr_index = ARM_INSTRUCTION_SXTB16;//tianman
										break;
									case 2:
										instr_index = ARM_INSTRUCTION_SXTB;//tianman
										break;
									case 3:
										instr_index = ARM_INSTRUCTION_SXTH;//tianman
										break;
									case 4:
										instr_index = ARM_INSTRUCTION_UXTB16;//tianman
										break;
									case 6:
										instr_index = ARM_INSTRUCTION_UXTB;//tianman
										break;
									case 7:
										instr_index = ARM_INSTRUCTION_UXTH;//tianman
										break;
									default: goto illegal_op;
								}
								if (rn != 15) {
									instr_index -= 3;//tianman
								}
								s->tb->extra_tb_info.arm_count[instr_index]+=1;//tianman
								return arm_instr_time[instr_index];
							} else if ((insn & 0x003f0f60) == 0x003f0f20) {
								/* rev */
								if (insn & (1 << 22)) {
									if (insn & (1 << 7)) {
									} else {
									}
								} else {
									if (insn & (1 << 7)){
										s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_REV16]+=1;//tianman
										return arm_instr_time[ARM_INSTRUCTION_REV16];
									}
									else{
										s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_REV]+=1;//tianman
										return arm_instr_time[ARM_INSTRUCTION_REV];
									}
								}
							} else {
								goto illegal_op;
							}
							break;
						case 2: /* Multiplies (Type 3).  */
							if (insn & (1 << 20)) {
								/* Signed multiply most significant [accumulate].  */
								if (rd != 15) {
									if (insn & (1 << 6)) {
										s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_SMMLS]+=1;//tianman
										return arm_instr_time[ARM_INSTRUCTION_SMMLS];
									} else {
										s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_SMMLA]+=1;//tianman
										return arm_instr_time[ARM_INSTRUCTION_SMMLA];
									}
								}
								else{
									s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_SMMUL]+=1;//tianman
									return arm_instr_time[ARM_INSTRUCTION_SMMUL];
								}
							} else {
								/* This addition cannot overflow.  */
								if (insn & (1 << 6)) {
									instr_index = 1;//tianman
								} else {
									instr_index = 0;//tianman
								}
								if (insn & (1 << 22)) {
									/* smlald, smlsld */
									instr_index += ARM_INSTRUCTION_SMLALD;//tianman
									s->tb->extra_tb_info.arm_count[instr_index]+=1;//tianman
									return arm_instr_time[instr_index];
								} else {
									/* smuad, smusd, smlad, smlsd */
									if (rd != 15){
										instr_index += 2;//tianman
									}
									instr_index += ARM_INSTRUCTION_SMUAD;//tianman
									s->tb->extra_tb_info.arm_count[instr_index]+=1;//tianman
									return arm_instr_time[instr_index];
								}
							}
							break;
						case 3:
							op1 = ((insn >> 17) & 0x38) | ((insn >> 5) & 7);
							switch (op1) {
								case 0: /* Unsigned sum of absolute differences.  */
									if (rd != 15) {
										s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_USADA8]+=1;//tianman
										return arm_instr_time[ARM_INSTRUCTION_USADA8];
									}
									s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_USAD8]+=1;//tianman
									return arm_instr_time[ARM_INSTRUCTION_USAD8];
									break;
								case 0x20: case 0x24: case 0x28: case 0x2c:
									/* Bitfield insert/clear.  */
									break;
								case 0x12: case 0x16: case 0x1a: case 0x1e: /* sbfx */
								case 0x32: case 0x36: case 0x3a: case 0x3e: /* ubfx */
									break;
								default:
									goto illegal_op;
							}
							break;
					}
					break;
				}
do_ldst_tianman:
				/* Check for undefined extension instructions
				 * per the ARM Bible IE:
				 * xxxx 0111 1111 xxxx  xxxx xxxx 1111 xxxx
				 */
				sh = (0xf << 20) | (0xf << 4);
				if (op1 == 0x7 && ((insn & sh) == sh))
				{
					goto illegal_op;
				}
				/* load/store byte/word */
				rn = (insn >> 16) & 0xf;
				rd = (insn >> 12) & 0xf;
				i = (IS_USER(s) || (insn & 0x01200000) == 0x00200000);
				if (insn & (1 << 20)) {
					/* load */
					if (insn & (1 << 22)) {
						s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_LDRB]+=1;//tianman
						return arm_instr_time[ARM_INSTRUCTION_LDRB];
					} else {
						s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_LDR]+=1;//tianman
						return arm_instr_time[ARM_INSTRUCTION_LDR];
					}
				} else {
					/* store */
					if (insn & (1 << 22))
					{
						s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_STRB]+=1;//tianman
						return arm_instr_time[ARM_INSTRUCTION_STRB];
					}
					else
					{
						s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_STR]+=1;//tianman
						return arm_instr_time[ARM_INSTRUCTION_STR];
					}
				}
//				if (insn & (1 << 20)) {
//					/* Complete the load.  */
//				}
				break;
			case 0x08:
			case 0x09:
				{
					int j, n, user, loaded_base;
					TCGv loaded_var;
					/* load/store multiple words */
					/* XXX: store correct base if write back */
					switch (insn & 0x00500000 >> 20) {//tianman
						case 0x0:
							s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_STM1]+=1;//tianman
							return arm_instr_time[ARM_INSTRUCTION_STM1];
							break;
						case 0x1:
							s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_LDM1]+=1;//tianman
							return arm_instr_time[ARM_INSTRUCTION_LDM1];
							break;
						case 0x4:
							s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_STM2]+=1;//tianman
							return arm_instr_time[ARM_INSTRUCTION_STM2];
							break;
						case 0x5:
							if (insn & (1 << 15)) {
								s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_LDM3]+=1;//tianman
								return arm_instr_time[ARM_INSTRUCTION_LDM3];
							}
							else {
								s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_LDM2]+=1;//tianman
								return arm_instr_time[ARM_INSTRUCTION_LDM2];
							}
							break;
					}
#if 0
					user = 0;
					if (insn & (1 << 22)) {
						if (IS_USER(s))
							goto illegal_op; /* only usable in supervisor mode */

						if ((insn & (1 << 15)) == 0)
							user = 1;
					}
					rn = (insn >> 16) & 0xf;
					addr = load_reg(s, rn);

					/* compute total size */
					loaded_base = 0;
					TCGV_UNUSED(loaded_var);
					n = 0;
					for(i=0;i<16;i++) {
						if (insn & (1 << i))
							n++;
					}
					/* XXX: test invalid n == 0 case ? */
					if (insn & (1 << 23)) {
						if (insn & (1 << 24)) {
							/* pre increment */
							tcg_gen_addi_i32(addr, addr, 4);
						} else {
							/* post increment */
						}
					} else {
						if (insn & (1 << 24)) {
							/* pre decrement */
							tcg_gen_addi_i32(addr, addr, -(n * 4));
						} else {
							/* post decrement */
							if (n != 1)
								tcg_gen_addi_i32(addr, addr, -((n - 1) * 4));
						}
					}
					if(insn & (1 << 20)) {
						/* paslab : load instructions counter += 1 */
						s->tb->extra_tb_info.load_count += 1;
					} else {
						/* paslab : store instructions counter += 1 */
						s->tb->extra_tb_info.store_count += 1;
					}
					j = 0;
					for(i=0;i<16;i++) {
						if (insn & (1 << i)) {
							if (insn & (1 << 20)) {
								/* load */
								if (i == 15) {
								} else if (user) {
								} else if (i == rn) {
								} else {
								}
							} else {
								/* store */
								if (i == 15) {
									/* special case: r15 = PC + 8 */
								} else if (user) {
								} else {
								}
							}
							j++;
							/* no need to add after the last transfer */
						}
					}
					if (insn & (1 << 21)) {
						/* write back */
						if (insn & (1 << 23)) {
							if (insn & (1 << 24)) {
								/* pre increment */
							} else {
								/* post increment */
							}
						} else {
							if (insn & (1 << 24)) {
								/* pre decrement */
								if (n != 1)
							} else {
								/* post decrement */
							}
						}
					} else {
					}
					if ((insn & (1 << 22)) && !user) {
						/* Restore CPSR from SPSR.  */
					}
#endif
				}
				break;
			case 0xa:
			case 0xb:
				{
					/* paslab : support event 0x05 (B & BL count) */
					s->tb->extra_tb_info.branch_count += 1;
					int32_t offset;

					/* branch (and link) */
					if (insn & (1 << 24)) {
						s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_B]+=1;//tianman
						return arm_instr_time[ARM_INSTRUCTION_B];
					}
					else {
						s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_B]+=1;//tianman
						return arm_instr_time[ARM_INSTRUCTION_B];
					}

				}
				break;
			case 0xc:
			case 0xd:
			case 0xe:
				/* Coprocessor.  */
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_COPROCESSOR]+=1;//tianman
				return arm_instr_time[ARM_INSTRUCTION_COPROCESSOR];
				break;
			case 0xf:
				/* swi */
				s->tb->extra_tb_info.arm_count[ARM_INSTRUCTION_SWI]+=1;//tianman
				return arm_instr_time[ARM_INSTRUCTION_SWI];
				break;
			default:
illegal_op:
				break;
		}
	}
	//if can't disdinguish the instruction then return 1
	return 1;
}

/* Implement by evo0209
 * Check dual issue possibility, and reduce redundant ticks caculate
 * by analyze_arm_ticks if the two instruction could be dual issued
 */
void dual_issue_check(DisasContext *s)
{
		//unsigned int pipe0_op1 = (s->tb->extra_tb_info.insn_buf[0] >> 21) & 0xf;
		//unsigned int pipe1_op1 = (s->tb->extra_tb_info.insn_buf[1] >> 21) & 0xf;
		uint64_t tmp_insn;
		int op1;
		int is_ls[2];
		int is_arith[2];
		int Rn[2];
		int Rm[2];
		int Rd[2];
		//int Rs[2] = {0};
		//int Rt[2] = {0};

		int i;
		for (i = 0 ; i < 2; i++)
		{
			tmp_insn = s->tb->extra_tb_info.insn_buf[i];
			if ((tmp_insn & 0x0f900000) == 0x03000000)
			{
				;
			}
			else if ((tmp_insn & 0x0f900000) == 0x01000000
					&& (tmp_insn & 0x00000090) != 0x00000090) 
			{
				/* miscellaneous instructions */
				;
			}
			else if (((tmp_insn & 0x0e000000) == 0 &&
						(tmp_insn & 0x00000090) != 0x90) ||
					((tmp_insn & 0x0e000000) == (1 << 25)))
			{
				Rd[i] = (tmp_insn >> 12) & 0xf;
				Rn[i] = (tmp_insn >> 16) & 0xf;
				Rm[i] = tmp_insn & 0xf;
				is_arith[i] = 1;
			}
			else {
				/* other instructions */
				op1 = (tmp_insn >> 24) & 0xf;
				Rd[i] = (tmp_insn >> 12) & 0xf;
				Rn[i] = (tmp_insn >> 16) & 0xf;
				if (op1 == 0x4 || op1 == 0x5)
					is_ls[i] = 1;
			}
		}

		/*if (is_arith[0] == 1 && is_arith[1] == 1)
		{
			if (Rd[0] != Rd[1] &&
				Rd[0] != Rm[1] && Rd[0] != Rn[1] &&
				Rd[1] != Rm[0] && Rd[1] != Rn[0])
				s->tb->extra_tb_info.dual_issue_reduce_ticks++;
		}
		else */if (is_arith[0] == 1 && is_ls[1] == 1)
		{
			if (Rd[0] != Rd[1] && Rd[0] != Rn[1] &&
				Rm[0] != Rd[1] && Rn[0] != Rd[1])
				s->tb->extra_tb_info.dual_issue_reduce_ticks++;
		}
		else if (is_arith[1] == 1 && is_ls[0] == 1)
		{
			if (Rd[1] != Rd[0] && Rd[1] != Rn[0] &&
				Rm[1] != Rd[0] && Rn[1] != Rd[0])
				s->tb->extra_tb_info.dual_issue_reduce_ticks++;
		}

		s->tb->extra_tb_info.insn_buf_index = 0;
		
		//for (i = 0; i < 2; i++)
		//{
		//	is_ls[i] = 0;
		//	is_arith[i] = 0;
		//	Rn[i] = 0;
		//	Rm[i] = 0;
		//	Rd[i] = 0;
		//	//Rs[2] = 0;
		//	//Rt[2] = 0;
		//}
}
