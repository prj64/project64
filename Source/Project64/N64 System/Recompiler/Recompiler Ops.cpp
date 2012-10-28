#include "stdafx.h"

CCodeSection * CRecompilerOps::m_Section = NULL;
CRegInfo	   CRecompilerOps::m_RegWorkingSet;
STEP_TYPE      CRecompilerOps::m_NextInstruction;
DWORD          CRecompilerOps::m_CompilePC;
OPCODE         CRecompilerOps::m_Opcode;
DWORD          CRecompilerOps::m_BranchCompare = 0;

void CRecompilerOps::CompileReadTLBMiss (int AddressReg, int LookUpReg ) 
{
#ifdef tofix
	MoveX86regToVariable(AddressReg,&TLBLoadAddress,"TLBLoadAddress");
	TestX86RegToX86Reg(LookUpReg,LookUpReg);
	m_Section->CompileExit(m_CompilePC, m_CompilePC,m_RegWorkingSet,CExitInfo::TLBReadMiss,FALSE,JeLabel32);
#endif
}

void CRecompilerOps::CompileWriteTLBMiss (int AddressReg, int LookUpReg ) 
{
#ifdef tofix
	MoveX86regToVariable(AddressReg,&TLBStoreAddress,"TLBStoreAddress");
	TestX86RegToX86Reg(LookUpReg,LookUpReg);
	m_Section->CompileExit(m_CompilePC, m_CompilePC,m_RegWorkingSet,CExitInfo::TLBReadMiss,FALSE,JeLabel32);
#endif
}

int  DelaySlotEffectsCompare ( DWORD PC, DWORD Reg1, DWORD Reg2 );

/************************** Branch functions  ************************/
void CRecompilerOps::Compile_Branch (CRecompilerOps::BranchFunction CompareFunc, BRANCH_TYPE BranchType, BOOL Link)
{
	static int EffectDelaySlot, DoneJumpDelay, DoneContinueDelay;
	static CRegInfo RegBeforeDelay;

	if ( m_NextInstruction == NORMAL ) {
		CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
		
		if ((m_CompilePC & 0xFFC) != 0xFFC) {
			switch (BranchType) {
			case BranchTypeRs: EffectDelaySlot = DelaySlotEffectsCompare(m_CompilePC,m_Opcode.rs,0); break;
			case BranchTypeRsRt: EffectDelaySlot = DelaySlotEffectsCompare(m_CompilePC,m_Opcode.rs,m_Opcode.rt); break;
			case BranchTypeCop1: 
				{
					OPCODE Command;

					if (!_MMU ->LW_VAddr(m_CompilePC + 4, Command.Hex)) {
						DisplayError(GS(MSG_FAIL_LOAD_WORD));
						ExitThread(0);
					}
					
					EffectDelaySlot = FALSE;
					if (Command.op == R4300i_CP1) {
						if (Command.fmt == R4300i_COP1_S && (Command.funct & 0x30) == 0x30 ) {
							EffectDelaySlot = TRUE;
						} 
						if (Command.fmt == R4300i_COP1_D && (Command.funct & 0x30) == 0x30 ) {
							EffectDelaySlot = TRUE;
						} 
					}
				}
				break;
#ifndef EXTERNAL_RELEASE
			default:
				DisplayError("Unknown branch type");
#endif
			}
		} else {
			EffectDelaySlot = TRUE;
		}
		m_Section->m_Jump.TargetPC        = m_CompilePC + ((short)m_Opcode.offset << 2) + 4;
		if (m_Section->m_JumpSection != NULL) {
			m_Section->m_Jump.BranchLabel.Format("Section_%d",m_Section->m_JumpSection->m_SectionID);
		} else {
			m_Section->m_Jump.BranchLabel.Format("Exit_%X_jump_%X",m_Section->m_EnterPC,m_Section->m_Jump.TargetPC);
		}
		m_Section->m_Jump.LinkLocation    = NULL;
		m_Section->m_Jump.LinkLocation2   = NULL;
		m_Section->m_Jump.DoneDelaySlot   = FALSE;
		m_Section->m_Cont.TargetPC        = m_CompilePC + 8;
		if (m_Section->m_ContinueSection != NULL) {
			m_Section->m_Cont.BranchLabel.Format("Section_%d",m_Section->m_ContinueSection->m_SectionID);
		} else {
			m_Section->m_Cont.BranchLabel.Format("Exit_%X_continue_%X",m_Section->m_EnterPC,m_Section->m_Cont.TargetPC);
		}
		m_Section->m_Cont.LinkLocation    = NULL;
		m_Section->m_Cont.LinkLocation2   = NULL;
		m_Section->m_Cont.DoneDelaySlot   = FALSE;
		if (m_Section->m_Jump.TargetPC < m_Section->m_Cont.TargetPC) {
			m_Section->m_Cont.FallThrough = FALSE;
			m_Section->m_Jump.FallThrough = TRUE;
		} else {
			m_Section->m_Cont.FallThrough = TRUE;
			m_Section->m_Jump.FallThrough = FALSE;
		}
		if (Link) {
			UnMap_GPR( 31, FALSE);
			MipsRegLo(31) = m_CompilePC + 8;
			MipsRegState(31) = CRegInfo::STATE_CONST_32;
		}
		if (EffectDelaySlot) {
			m_Section->m_Cont.BranchLabel = m_Section->m_ContinueSection != NULL ? "Continue" : "ExitBlock";
			m_Section->m_Jump.BranchLabel = m_Section->m_JumpSection != NULL ? "Jump" : "ExitBlock";
			CompareFunc(); 
			
			if ((m_CompilePC & 0xFFC) == 0xFFC) {
				m_Section->GenerateSectionLinkage();
				m_NextInstruction = END_BLOCK;
				return;
			}
			if (!m_Section->m_Jump.FallThrough && !m_Section->m_Cont.FallThrough) {
				if (m_Section->m_Jump.LinkLocation != NULL) {
					CPU_Message("");
					CPU_Message("      %s:",m_Section->m_Jump.BranchLabel.c_str());
					SetJump32((DWORD *)m_Section->m_Jump.LinkLocation,(DWORD *)m_RecompPos);
					m_Section->m_Jump.LinkLocation = NULL;
					if (m_Section->m_Jump.LinkLocation2 != NULL) {
						SetJump32((DWORD *)m_Section->m_Jump.LinkLocation2,(DWORD *)m_RecompPos);
						m_Section->m_Jump.LinkLocation2 = NULL;
					}
					m_Section->m_Jump.FallThrough = TRUE;
				} else if (m_Section->m_Cont.LinkLocation != NULL){
					CPU_Message("");
					CPU_Message("      %s:",m_Section->m_Cont.BranchLabel.c_str());
					SetJump32((DWORD *)m_Section->m_Cont.LinkLocation,(DWORD *)m_RecompPos);
					m_Section->m_Cont.LinkLocation = NULL;
					if (m_Section->m_Cont.LinkLocation2 != NULL) {
						SetJump32((DWORD *)m_Section->m_Cont.LinkLocation2,(DWORD *)m_RecompPos);
						m_Section->m_Cont.LinkLocation2 = NULL;
					}
					m_Section->m_Cont.FallThrough = TRUE;
				}
			}
			ResetX86Protection();
			memcpy(&RegBeforeDelay,&m_RegWorkingSet,sizeof(CRegInfo));
		}
		m_NextInstruction = DO_DELAY_SLOT;
	} else if (m_NextInstruction == DELAY_SLOT_DONE ) {		
		if (EffectDelaySlot) { 
			CJumpInfo * FallInfo = m_Section->m_Jump.FallThrough?&m_Section->m_Jump:&m_Section->m_Cont;
			CJumpInfo * JumpInfo = m_Section->m_Jump.FallThrough?&m_Section->m_Cont:&m_Section->m_Jump;

			if (FallInfo->FallThrough && !FallInfo->DoneDelaySlot) {
				ResetX86Protection();
				FallInfo->RegSet = m_RegWorkingSet;
				if (FallInfo == &m_Section->m_Jump) {
					if (m_Section->m_JumpSection != NULL) {
						m_Section->m_Jump.BranchLabel.Format("Section_%d",m_Section->m_JumpSection->m_SectionID);
					} else {
						m_Section->m_Jump.BranchLabel = "ExitBlock";
					}
					if (FallInfo->TargetPC <= m_CompilePC) 
					{
						_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
						UpdateCounters(&(FallInfo->RegSet.BlockCycleCount()),&(FallInfo->RegSet.BlockRandomModifier()),true);
						m_Section->CompileSystemCheck(FallInfo->TargetPC,FallInfo->RegSet);
						ResetX86Protection();
#endif
					}
				} else {
					if (m_Section->m_ContinueSection != NULL) {
						m_Section->m_Cont.BranchLabel.Format("Section_%d",m_Section->m_ContinueSection->m_SectionID);
					} else {
						m_Section->m_Cont.BranchLabel = "ExitBlock";
					}
				}		
				FallInfo->DoneDelaySlot = TRUE;
				if (!JumpInfo->DoneDelaySlot) {
					FallInfo->FallThrough = FALSE;				
					JmpLabel32(FallInfo->BranchLabel.c_str(),0);
					FallInfo->LinkLocation = (DWORD *)(m_RecompPos - 4);
					
					if (JumpInfo->LinkLocation != NULL) {
						CPU_Message("      %s:",JumpInfo->BranchLabel.c_str());
						SetJump32((DWORD *)JumpInfo->LinkLocation,(DWORD *)m_RecompPos);
						JumpInfo->LinkLocation = NULL;
						if (JumpInfo->LinkLocation2 != NULL) {
							SetJump32((DWORD *)JumpInfo->LinkLocation2,(DWORD *)m_RecompPos);
							JumpInfo->LinkLocation2 = NULL;
						}
						JumpInfo->FallThrough = TRUE;
						m_NextInstruction = DO_DELAY_SLOT;
						memcpy(&m_RegWorkingSet,&RegBeforeDelay,sizeof(CRegInfo));
						return; 
					}
				}
			}
		} else {
			CompareFunc();
			ResetX86Protection();
			memcpy(&m_Section->m_Cont.RegSet,&m_RegWorkingSet,sizeof(CRegInfo));
			memcpy(&m_Section->m_Jump.RegSet,&m_RegWorkingSet,sizeof(CRegInfo));
		}
		m_Section->GenerateSectionLinkage();
		m_NextInstruction = END_BLOCK;
	} else {
#ifndef EXTERNAL_RELEASE
		DisplayError("WTF\n\nBranch\nNextInstruction = %X", m_NextInstruction);
#endif
	}
}

void CRecompilerOps::Compile_BranchLikely (BranchFunction CompareFunc, BOOL Link)
{
	if ( m_NextInstruction == NORMAL ) {		
		CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
		
		m_Section->m_Jump.TargetPC      = m_CompilePC + ((short)m_Opcode.offset << 2) + 4;
		if (m_Section->m_JumpSection != NULL) {
			m_Section->m_Jump.BranchLabel.Format("Section_%d",((CCodeSection *)m_Section->m_JumpSection)->m_SectionID);
		} else {
			m_Section->m_Jump.BranchLabel = "ExitBlock";
		}
		m_Section->m_Jump.FallThrough   = TRUE;
		m_Section->m_Jump.LinkLocation  = NULL;
		m_Section->m_Jump.LinkLocation2 = NULL;
		m_Section->m_Cont.TargetPC      = m_CompilePC + 8;
		if (m_Section->m_ContinueSection != NULL) {
			m_Section->m_Cont.BranchLabel.Format("Section_%d",((CCodeSection *)m_Section->m_ContinueSection)->m_SectionID);
		} else {
			m_Section->m_Cont.BranchLabel = "ExitBlock";
		}
		m_Section->m_Cont.FallThrough   = FALSE;
		m_Section->m_Cont.LinkLocation  = NULL;
		m_Section->m_Cont.LinkLocation2 = NULL;
		if (Link) {
			UnMap_GPR( 31, FALSE);
			MipsRegLo(31) = m_CompilePC + 8;
			MipsRegState(31) = CRegInfo::STATE_CONST_32;
		}
		CompareFunc(); 
		ResetX86Protection();
		memcpy(&m_Section->m_Cont.RegSet,&m_RegWorkingSet,sizeof(CRegInfo));
		if (g_UseLinking && m_Section->m_Jump.TargetPC == m_Section->m_Cont.TargetPC)
		{
			_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
			if (m_Section->m_Cont.FallThrough)  
			{
				BreakPoint(__FILE__,__LINE__);
			}
			if (!m_Section->m_Jump.FallThrough)
			{
				BreakPoint(__FILE__,__LINE__);
			}
			m_Section->m_JumpSection->m_Cont.TargetPC = m_Section->m_Jump.TargetPC;
			m_Section->m_JumpSection->DelaySlotSection = true;
			m_Section->m_Jump.TargetPC = m_CompilePC + 4;
			m_Section->m_Jump.RegSet = m_RegWorkingSet;
			m_Section->GenerateSectionLinkage();
			m_NextInstruction = END_BLOCK;
#endif
		} else {
			if (m_Section->m_Cont.FallThrough)  {
				if (m_Section->m_Jump.LinkLocation != NULL) {
	#ifndef EXTERNAL_RELEASE
					DisplayError("WTF .. problem with CRecompilerOps::BranchLikely");
	#endif
				}
				m_Section->GenerateSectionLinkage();
				m_NextInstruction = END_BLOCK;
			} else {
				if ((m_CompilePC & 0xFFC) == 0xFFC) {
					m_Section->m_Jump.FallThrough = FALSE;
					if (m_Section->m_Jump.LinkLocation != NULL) {
						SetJump32(m_Section->m_Jump.LinkLocation,(DWORD *)m_RecompPos);
						m_Section->m_Jump.LinkLocation = NULL;
						if (m_Section->m_Jump.LinkLocation2 != NULL) { 
							SetJump32(m_Section->m_Jump.LinkLocation2,(DWORD *)m_RecompPos);
							m_Section->m_Jump.LinkLocation2 = NULL;
						}
					}
					JmpLabel32("DoDelaySlot",0);
					m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
					CPU_Message("      ");
					CPU_Message("      %s:",m_Section->m_Cont.BranchLabel.c_str());
					if (m_Section->m_Cont.LinkLocation != NULL) {
						SetJump32(m_Section->m_Cont.LinkLocation,(DWORD *)m_RecompPos);
						m_Section->m_Cont.LinkLocation = NULL;
						if (m_Section->m_Cont.LinkLocation2 != NULL) { 
							SetJump32(m_Section->m_Cont.LinkLocation2,(DWORD *)m_RecompPos);
							m_Section->m_Cont.LinkLocation2 = NULL;
						}
					}
					m_Section->CompileExit(m_CompilePC, m_CompilePC + 8,m_RegWorkingSet,CExitInfo::Normal,TRUE,NULL);
					CPU_Message("      ");
					CPU_Message("      DoDelaySlot");
					m_Section->GenerateSectionLinkage();
					m_NextInstruction = END_BLOCK;
				} else {
					m_NextInstruction = DO_DELAY_SLOT;
				}
			}
		}
	} else if (m_NextInstruction == DELAY_SLOT_DONE ) {
		ResetX86Protection();
		memcpy(&m_Section->m_Jump.RegSet,&m_RegWorkingSet,sizeof(CRegInfo));
		m_Section->GenerateSectionLinkage();
		m_NextInstruction = END_BLOCK;
	} else {
#ifndef EXTERNAL_RELEASE
		DisplayError("WTF\n\nBranchLikely\nNextInstruction = %X", m_NextInstruction);
#endif
	}
}

void CRecompilerOps::BNE_Compare (void) 
{
	BYTE *Jump;

	if (m_Section->IsKnown(m_Opcode.rs) && m_Section->IsKnown(m_Opcode.rt)) {
		if (IsConst(m_Opcode.rs) && IsConst(m_Opcode.rt)) {
			if (Is64Bit(m_Opcode.rs) || Is64Bit(m_Opcode.rt)) {
				CRecompilerOps::UnknownOpcode();
			} else if (cMipsRegLo(m_Opcode.rs) != cMipsRegLo(m_Opcode.rt)) {
				m_Section->m_Jump.FallThrough = TRUE;
				m_Section->m_Cont.FallThrough = FALSE;
			} else {
				m_Section->m_Jump.FallThrough = FALSE;
				m_Section->m_Cont.FallThrough = TRUE;
			}
		} else if (IsMapped(m_Opcode.rs) && IsMapped(m_Opcode.rt)) {
			if (Is64Bit(m_Opcode.rs) || Is64Bit(m_Opcode.rt)) {
				ProtectGPR(m_Opcode.rs);
				ProtectGPR(m_Opcode.rt);

				CompX86RegToX86Reg(
					m_Section->Is32Bit(m_Opcode.rs)?Map_TempReg(x86_Any,m_Opcode.rs,TRUE):MipsRegMapHi(m_Opcode.rs),
					m_Section->Is32Bit(m_Opcode.rt)?Map_TempReg(x86_Any,m_Opcode.rt,TRUE):MipsRegMapHi(m_Opcode.rt)
				);
					
				if (m_Section->m_Jump.FallThrough) {
					JneLabel8("continue",0);
					Jump = m_RecompPos - 1;
				} else {
					JneLabel32(m_Section->m_Jump.BranchLabel.c_str(),0);
					m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
				}
				CompX86RegToX86Reg(cMipsRegMapLo(m_Opcode.rs),cMipsRegMapLo(m_Opcode.rt));
				if (m_Section->m_Cont.FallThrough) {
					JneLabel32 ( m_Section->m_Jump.BranchLabel.c_str(), 0 );
					m_Section->m_Jump.LinkLocation2 = (DWORD *)(m_RecompPos - 4);
				} else if (m_Section->m_Jump.FallThrough) {
					JeLabel32 ( m_Section->m_Cont.BranchLabel.c_str(), 0 );
					m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
					CPU_Message("      ");
					CPU_Message("      continue:");
					SetJump8(Jump,m_RecompPos);
				} else {
					JeLabel32 ( m_Section->m_Cont.BranchLabel.c_str(), 0 );
					m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
					JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(),0);
					m_Section->m_Jump.LinkLocation2 = (DWORD *)(m_RecompPos - 4);
				}
			} else {
				CompX86RegToX86Reg(cMipsRegMapLo(m_Opcode.rs),cMipsRegMapLo(m_Opcode.rt));
				if (m_Section->m_Cont.FallThrough) {
					JneLabel32 ( m_Section->m_Jump.BranchLabel.c_str(), 0 );
					m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
				} else if (m_Section->m_Jump.FallThrough) {
					JeLabel32 ( m_Section->m_Cont.BranchLabel.c_str(), 0 );
					m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
				} else {
					JeLabel32 ( m_Section->m_Cont.BranchLabel.c_str(), 0 );
					m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
					JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(),0);
					m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
				}
			}
		} else {
			DWORD ConstReg = IsConst(m_Opcode.rt)?m_Opcode.rt:m_Opcode.rs;
			DWORD MappedReg = IsConst(m_Opcode.rt)?m_Opcode.rs:m_Opcode.rt;

			if (Is64Bit(ConstReg) || Is64Bit(MappedReg)) {
				if (m_Section->Is32Bit(ConstReg) || m_Section->Is32Bit(MappedReg)) {
					ProtectGPR(MappedReg);
					if (m_Section->Is32Bit(MappedReg)) {
						CompConstToX86reg(Map_TempReg(x86_Any,MappedReg,TRUE),MipsRegHi(ConstReg));
					} else {
						CompConstToX86reg(MipsRegMapHi(MappedReg),cMipsRegLo_S(ConstReg) >> 31);
					}
				} else {
					CompConstToX86reg(MipsRegMapHi(MappedReg),MipsRegHi(ConstReg));
				}
				if (m_Section->m_Jump.FallThrough) {
					JneLabel8("continue",0);
					Jump = m_RecompPos - 1;
				} else {
					JneLabel32(m_Section->m_Jump.BranchLabel.c_str(),0);
					m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
				}
				CompConstToX86reg(cMipsRegMapLo(MappedReg),cMipsRegLo(ConstReg));
				if (m_Section->m_Cont.FallThrough) {
					JneLabel32 ( m_Section->m_Jump.BranchLabel.c_str(), 0 );
					m_Section->m_Jump.LinkLocation2 = (DWORD *)(m_RecompPos - 4);
				} else if (m_Section->m_Jump.FallThrough) {
					JeLabel32 ( m_Section->m_Cont.BranchLabel.c_str(), 0 );
					m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
					CPU_Message("      ");
					CPU_Message("      continue:");
					SetJump8(Jump,m_RecompPos);
				} else {
					JeLabel32 ( m_Section->m_Cont.BranchLabel.c_str(), 0 );
					m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
					JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(),0);
					m_Section->m_Jump.LinkLocation2 = (DWORD *)(m_RecompPos - 4);
				}
			} else {
				CompConstToX86reg(cMipsRegMapLo(MappedReg),cMipsRegLo(ConstReg));
				if (m_Section->m_Cont.FallThrough) {
					JneLabel32 ( m_Section->m_Jump.BranchLabel.c_str(), 0 );
					m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
				} else if (m_Section->m_Jump.FallThrough) {
					JeLabel32 ( m_Section->m_Cont.BranchLabel.c_str(), 0 );
					m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
				} else {
					JeLabel32 ( m_Section->m_Cont.BranchLabel.c_str(), 0 );
					m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
					JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(),0);
					m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
				}
			}
		}
	} else if (m_Section->IsKnown(m_Opcode.rs) || m_Section->IsKnown(m_Opcode.rt)) {
		DWORD KnownReg = m_Section->IsKnown(m_Opcode.rt)?m_Opcode.rt:m_Opcode.rs;
		DWORD UnknownReg = m_Section->IsKnown(m_Opcode.rt)?m_Opcode.rs:m_Opcode.rt;

		if (IsConst(KnownReg)) {
			if (Is64Bit(KnownReg)) {
				CompConstToVariable(MipsRegHi(KnownReg),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else if (m_Section->IsSigned(KnownReg)) {
				CompConstToVariable((cMipsRegLo_S(KnownReg) >> 31),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else {
				CompConstToVariable(0,&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			}
		} else {
			if (Is64Bit(KnownReg)) {
				CompX86regToVariable(MipsRegMapHi(KnownReg),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else if (m_Section->IsSigned(KnownReg)) {
				ProtectGPR(KnownReg);
				CompX86regToVariable(Map_TempReg(x86_Any,KnownReg,TRUE),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else {
				CompConstToVariable(0,&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			}
		}
		if (m_Section->m_Jump.FallThrough) {
			JneLabel8("continue",0);
			Jump = m_RecompPos - 1;
		} else {
			JneLabel32(m_Section->m_Jump.BranchLabel.c_str(),0);
			m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
		}
		if (IsConst(KnownReg)) {
			CompConstToVariable(cMipsRegLo(KnownReg),&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg]);
		} else {
			CompX86regToVariable(cMipsRegMapLo(KnownReg),&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg]);
		}
		if (m_Section->m_Cont.FallThrough) {
			JneLabel32 ( m_Section->m_Jump.BranchLabel.c_str(), 0 );
			m_Section->m_Jump.LinkLocation2 = (DWORD *)(m_RecompPos - 4);
		} else if (m_Section->m_Jump.FallThrough) {
			JeLabel32 ( m_Section->m_Cont.BranchLabel.c_str(), 0 );
			m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
			CPU_Message("      ");
			CPU_Message("      continue:");
			SetJump8(Jump,m_RecompPos);
		} else {
			JeLabel32 ( m_Section->m_Cont.BranchLabel.c_str(), 0 );
			m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
			JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(),0);
			m_Section->m_Jump.LinkLocation2 = (DWORD *)(m_RecompPos - 4);
		}
	} else {
		x86Reg Reg;

		Reg = Map_TempReg(x86_Any,m_Opcode.rt,TRUE);		
		CompX86regToVariable(Reg,&_GPR[m_Opcode.rs].W[1],CRegName::GPR_Hi[m_Opcode.rs]);
		if (m_Section->m_Jump.FallThrough) {
			JneLabel8("continue",0);
			Jump = m_RecompPos - 1;
		} else {
			JneLabel32(m_Section->m_Jump.BranchLabel.c_str(),0);
			m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
		}

		Reg = Map_TempReg(Reg,m_Opcode.rt,FALSE);
		CompX86regToVariable(Reg,&_GPR[m_Opcode.rs].W[0],CRegName::GPR_Lo[m_Opcode.rs]);
		if (m_Section->m_Cont.FallThrough) {
			JneLabel32 ( m_Section->m_Jump.BranchLabel.c_str(), 0 );
			m_Section->m_Jump.LinkLocation2 = (DWORD *)(m_RecompPos - 4);
		} else if (m_Section->m_Jump.FallThrough) {
			JeLabel32 ( m_Section->m_Cont.BranchLabel.c_str(), 0 );
			m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
			CPU_Message("      ");
			CPU_Message("      continue:");
			SetJump8(Jump,m_RecompPos);
		} else {
			JeLabel32 ( m_Section->m_Cont.BranchLabel.c_str(), 0 );
			m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
			JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(),0);
			m_Section->m_Jump.LinkLocation2 = (DWORD *)(m_RecompPos - 4);
		}
	}
}

void CRecompilerOps::BEQ_Compare (void) {
	BYTE *Jump;

	if (m_Section->IsKnown(m_Opcode.rs) && m_Section->IsKnown(m_Opcode.rt)) {
		if (IsConst(m_Opcode.rs) && IsConst(m_Opcode.rt)) {
			if (Is64Bit(m_Opcode.rs) || Is64Bit(m_Opcode.rt)) {
				CRecompilerOps::UnknownOpcode();
			} else if (cMipsRegLo(m_Opcode.rs) == cMipsRegLo(m_Opcode.rt)) {
				m_Section->m_Jump.FallThrough = TRUE;
				m_Section->m_Cont.FallThrough = FALSE;
			} else {
				m_Section->m_Jump.FallThrough = FALSE;
				m_Section->m_Cont.FallThrough = TRUE;
			}
		} else if (IsMapped(m_Opcode.rs) && IsMapped(m_Opcode.rt)) {
			if (Is64Bit(m_Opcode.rs) || Is64Bit(m_Opcode.rt)) {
				ProtectGPR(m_Opcode.rs);
				ProtectGPR(m_Opcode.rt);

				CompX86RegToX86Reg(
					m_Section->Is32Bit(m_Opcode.rs)?Map_TempReg(x86_Any,m_Opcode.rs,TRUE):MipsRegMapHi(m_Opcode.rs),
					m_Section->Is32Bit(m_Opcode.rt)?Map_TempReg(x86_Any,m_Opcode.rt,TRUE):MipsRegMapHi(m_Opcode.rt)
				);
				if (m_Section->m_Cont.FallThrough) {
					JneLabel8("continue",0);
					Jump = m_RecompPos - 1;
				} else {
					JneLabel32(m_Section->m_Cont.BranchLabel.c_str(),0);
					m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
				}
				CompX86RegToX86Reg(cMipsRegMapLo(m_Opcode.rs),cMipsRegMapLo(m_Opcode.rt));
				if (m_Section->m_Cont.FallThrough) {
					JeLabel32 ( m_Section->m_Jump.BranchLabel.c_str(), 0 );
					m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
					CPU_Message("      ");
					CPU_Message("      continue:");
					SetJump8(Jump,m_RecompPos);
				} else if (m_Section->m_Jump.FallThrough) {
					JneLabel32 ( m_Section->m_Cont.BranchLabel.c_str(), 0 );
					m_Section->m_Cont.LinkLocation2 = (DWORD *)(m_RecompPos - 4);
				} else {
					JneLabel32 ( m_Section->m_Cont.BranchLabel.c_str(), 0 );
					m_Section->m_Cont.LinkLocation2 = (DWORD *)(m_RecompPos - 4);
					JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(),0);
					m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
				}
			} else {
				CompX86RegToX86Reg(cMipsRegMapLo(m_Opcode.rs),cMipsRegMapLo(m_Opcode.rt));
				if (m_Section->m_Cont.FallThrough) {
					JeLabel32 ( m_Section->m_Jump.BranchLabel.c_str(), 0 );
					m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
				} else if (m_Section->m_Jump.FallThrough) {
					JneLabel32 ( m_Section->m_Cont.BranchLabel.c_str(), 0 );
					m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
				} else {
					JneLabel32 ( m_Section->m_Cont.BranchLabel.c_str(), 0 );
					m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
					JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(),0);
					m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
				}
			}
		} else {
			DWORD ConstReg = IsConst(m_Opcode.rt)?m_Opcode.rt:m_Opcode.rs;
			DWORD MappedReg = IsConst(m_Opcode.rt)?m_Opcode.rs:m_Opcode.rt;

			if (Is64Bit(ConstReg) || Is64Bit(MappedReg)) {
				if (m_Section->Is32Bit(ConstReg) || m_Section->Is32Bit(MappedReg)) {
					if (m_Section->Is32Bit(MappedReg)) {
						ProtectGPR(MappedReg);
						CompConstToX86reg(Map_TempReg(x86_Any,MappedReg,TRUE),MipsRegHi(ConstReg));
					} else {
						CompConstToX86reg(MipsRegMapHi(MappedReg),cMipsRegLo_S(ConstReg) >> 31);
					}
				} else {
					CompConstToX86reg(MipsRegMapHi(MappedReg),MipsRegHi(ConstReg));
				}			
				if (m_Section->m_Cont.FallThrough) {
					JneLabel8("continue",0);
					Jump = m_RecompPos - 1;
				} else {
					JneLabel32(m_Section->m_Cont.BranchLabel.c_str(),0);
					m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
				}
				CompConstToX86reg(cMipsRegMapLo(MappedReg),cMipsRegLo(ConstReg));
				if (m_Section->m_Cont.FallThrough) {
					JeLabel32 ( m_Section->m_Jump.BranchLabel.c_str(), 0 );
					m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
					CPU_Message("      ");
					CPU_Message("      continue:");
					SetJump8(Jump,m_RecompPos);
				} else if (m_Section->m_Jump.FallThrough) {
					JneLabel32 ( m_Section->m_Cont.BranchLabel.c_str(), 0 );
					m_Section->m_Cont.LinkLocation2 = (DWORD *)(m_RecompPos - 4);
				} else {
					JneLabel32 ( m_Section->m_Cont.BranchLabel.c_str(), 0 );
					m_Section->m_Cont.LinkLocation2 = (DWORD *)(m_RecompPos - 4);
					JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(),0);
					m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
				}
			} else {
				CompConstToX86reg(cMipsRegMapLo(MappedReg),cMipsRegLo(ConstReg));
				if (m_Section->m_Cont.FallThrough) {
					JeLabel32 ( m_Section->m_Jump.BranchLabel.c_str(), 0 );
					m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
				} else if (m_Section->m_Jump.FallThrough) {
					JneLabel32 ( m_Section->m_Cont.BranchLabel.c_str(), 0 );
					m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
				} else {
					JneLabel32 ( m_Section->m_Cont.BranchLabel.c_str(), 0 );
					m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
					JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(),0);
					m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
				}
			}
		}
	} else if (m_Section->IsKnown(m_Opcode.rs) || m_Section->IsKnown(m_Opcode.rt)) {
		DWORD KnownReg = m_Section->IsKnown(m_Opcode.rt)?m_Opcode.rt:m_Opcode.rs;
		DWORD UnknownReg = m_Section->IsKnown(m_Opcode.rt)?m_Opcode.rs:m_Opcode.rt;

		if (IsConst(KnownReg)) {
			if (Is64Bit(KnownReg)) {
				CompConstToVariable(MipsRegHi(KnownReg),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else if (m_Section->IsSigned(KnownReg)) {
				CompConstToVariable(cMipsRegLo_S(KnownReg) >> 31,&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else {
				CompConstToVariable(0,&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			}
		} else {
			ProtectGPR(KnownReg);
			if (Is64Bit(KnownReg)) {
				CompX86regToVariable(cMipsRegMapHi(KnownReg),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else if (m_Section->IsSigned(KnownReg)) {
				CompX86regToVariable(Map_TempReg(x86_Any,KnownReg,TRUE),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else {
				CompConstToVariable(0,&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			}
		}
		if (m_Section->m_Cont.FallThrough) {
			JneLabel8("continue",0);
			Jump = m_RecompPos - 1;
		} else {
			JneLabel32(m_Section->m_Cont.BranchLabel.c_str(),0);
			m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
		}
		if (IsConst(KnownReg)) {
			CompConstToVariable(cMipsRegLo(KnownReg),&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg]);
		} else {
			CompX86regToVariable(cMipsRegMapLo(KnownReg),&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg]);
		}
		if (m_Section->m_Cont.FallThrough) {
			JeLabel32 ( m_Section->m_Jump.BranchLabel.c_str(), 0 );
			m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
			CPU_Message("      ");
			CPU_Message("      continue:");
			SetJump8(Jump,m_RecompPos);
		} else if (m_Section->m_Jump.FallThrough) {
			JneLabel32 ( m_Section->m_Cont.BranchLabel.c_str(), 0 );
			m_Section->m_Cont.LinkLocation2 = (DWORD *)(m_RecompPos - 4);
		} else {
			JneLabel32 ( m_Section->m_Cont.BranchLabel.c_str(), 0 );
			m_Section->m_Cont.LinkLocation2 = (DWORD *)(m_RecompPos - 4);
			JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(),0);
			m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
		}
	} else {
		x86Reg Reg = Map_TempReg(x86_Any,m_Opcode.rs,TRUE);
		CompX86regToVariable(Reg,&_GPR[m_Opcode.rt].W[1],CRegName::GPR_Hi[m_Opcode.rt]);
		if (m_Section->m_Cont.FallThrough) {
			JneLabel8("continue",0);
			Jump = m_RecompPos - 1;
		} else {
			JneLabel32(m_Section->m_Cont.BranchLabel.c_str(),0);
			m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
		}
		CompX86regToVariable(Map_TempReg(Reg,m_Opcode.rs,FALSE),&_GPR[m_Opcode.rt].W[0],CRegName::GPR_Lo[m_Opcode.rt]);
		if (m_Section->m_Cont.FallThrough) {
			JeLabel32 ( m_Section->m_Jump.BranchLabel.c_str(), 0 );
			m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
			CPU_Message("      ");
			CPU_Message("      continue:");
			SetJump8(Jump,m_RecompPos);
		} else if (m_Section->m_Jump.FallThrough) {
			JneLabel32 ( m_Section->m_Cont.BranchLabel.c_str(), 0 );
			m_Section->m_Cont.LinkLocation2 = (DWORD *)(m_RecompPos - 4);
		} else {
			JneLabel32 ( m_Section->m_Cont.BranchLabel.c_str(), 0 );
			m_Section->m_Cont.LinkLocation2 = (DWORD *)(m_RecompPos - 4);
			JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(),0);
			m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
		}
	}
}

void CRecompilerOps::BGTZ_Compare (void) {
	if (IsConst(m_Opcode.rs)) {
		if (Is64Bit(m_Opcode.rs)) {
			if (m_Section->MipsReg_S(m_Opcode.rs) > 0) {
				m_Section->m_Jump.FallThrough = TRUE;
				m_Section->m_Cont.FallThrough = FALSE;
			} else {
				m_Section->m_Jump.FallThrough = FALSE;
				m_Section->m_Cont.FallThrough = TRUE;
			}
		} else {
			if (m_Section->MipsRegLo_S(m_Opcode.rs) > 0) {
				m_Section->m_Jump.FallThrough = TRUE;
				m_Section->m_Cont.FallThrough = FALSE;
			} else {
				m_Section->m_Jump.FallThrough = FALSE;
				m_Section->m_Cont.FallThrough = TRUE;
			}
		}
	} else if (IsMapped(m_Opcode.rs) && m_Section->Is32Bit(m_Opcode.rs)) {
		CompConstToX86reg(cMipsRegMapLo(m_Opcode.rs),0);
		if (m_Section->m_Jump.FallThrough) {
			JleLabel32 (m_Section->m_Cont.BranchLabel.c_str(), 0 );
			m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
		} else if (m_Section->m_Cont.FallThrough) {
			JgLabel32(m_Section->m_Jump.BranchLabel.c_str(),0);
			m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
		} else {
			JleLabel32 (m_Section->m_Cont.BranchLabel.c_str(), 0 );
			m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
			JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(),0);
			m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
		}
	} else {
		BYTE *Jump;

		if (IsMapped(m_Opcode.rs)) {
			CompConstToX86reg(MipsRegMapHi(m_Opcode.rs),0);
		} else {
			CompConstToVariable(0,&_GPR[m_Opcode.rs].W[1],CRegName::GPR_Hi[m_Opcode.rs]);
		}
		if (m_Section->m_Jump.FallThrough) {
			JlLabel32 (m_Section->m_Cont.BranchLabel.c_str(), 0 );
			m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
			JgLabel8("continue",0);
			Jump = m_RecompPos - 1;
		} else if (m_Section->m_Cont.FallThrough) {
			JlLabel8("continue",0);
			Jump = m_RecompPos - 1;
			JgLabel32(m_Section->m_Jump.BranchLabel.c_str(),0);
			m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
		} else {
			JlLabel32 (m_Section->m_Cont.BranchLabel.c_str(), 0 );
			m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
			JgLabel32(m_Section->m_Jump.BranchLabel.c_str(),0);
			m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
		}

		if (IsMapped(m_Opcode.rs)) {
			CompConstToX86reg(cMipsRegMapLo(m_Opcode.rs),0);
		} else {
			CompConstToVariable(0,&_GPR[m_Opcode.rs].W[0],CRegName::GPR_Lo[m_Opcode.rs]);
		}
		if (m_Section->m_Jump.FallThrough) {
			JeLabel32 (m_Section->m_Cont.BranchLabel.c_str(), 0 );
			m_Section->m_Cont.LinkLocation2 = (DWORD *)(m_RecompPos - 4);
			CPU_Message("      continue:");
			*((BYTE *)(Jump))=(BYTE)(m_RecompPos - Jump - 1);
		} else if (m_Section->m_Cont.FallThrough) {
			JneLabel32(m_Section->m_Jump.BranchLabel.c_str(),0);
			m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
			CPU_Message("      continue:");
			*((BYTE *)(Jump))=(BYTE)(m_RecompPos - Jump - 1);
		} else {
			JneLabel32(m_Section->m_Jump.BranchLabel.c_str(),0);
			m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
			JmpLabel32 (m_Section->m_Cont.BranchLabel.c_str(), 0 );
			m_Section->m_Cont.LinkLocation2 = (DWORD *)(m_RecompPos - 4);
		}
	}
}

void CRecompilerOps::BLEZ_Compare (void) {
	if (IsConst(m_Opcode.rs)) {
		if (Is64Bit(m_Opcode.rs)) {
			if (m_Section->MipsReg_S(m_Opcode.rs) <= 0) {
				m_Section->m_Jump.FallThrough = TRUE;
				m_Section->m_Cont.FallThrough = FALSE;
			} else {
				m_Section->m_Jump.FallThrough = FALSE;
				m_Section->m_Cont.FallThrough = TRUE;
			}
		} else if (m_Section->IsSigned(m_Opcode.rs)) {
			if (m_Section->MipsRegLo_S(m_Opcode.rs) <= 0) {
				m_Section->m_Jump.FallThrough = TRUE;
				m_Section->m_Cont.FallThrough = FALSE;
			} else {
				m_Section->m_Jump.FallThrough = FALSE;
				m_Section->m_Cont.FallThrough = TRUE;
			}
		} else {
			if (cMipsRegLo(m_Opcode.rs) == 0) {
				m_Section->m_Jump.FallThrough = TRUE;
				m_Section->m_Cont.FallThrough = FALSE;
			} else {
				m_Section->m_Jump.FallThrough = FALSE;
				m_Section->m_Cont.FallThrough = TRUE;
			}
		}
	} else {
		if (IsMapped(m_Opcode.rs) && m_Section->Is32Bit(m_Opcode.rs)) {
			CompConstToX86reg(cMipsRegMapLo(m_Opcode.rs),0);
			if (m_Section->m_Jump.FallThrough) {
				JgLabel32 (m_Section->m_Cont.BranchLabel.c_str(), 0 );
				m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
			} else if (m_Section->m_Cont.FallThrough) {
				JleLabel32(m_Section->m_Jump.BranchLabel.c_str(),0);
				m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
			} else {
				JgLabel32 (m_Section->m_Cont.BranchLabel.c_str(), 0 );
				m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
				JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(),0);
				m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
			}
		} else {
			BYTE *Jump;

			if (IsMapped(m_Opcode.rs)) {
				CompConstToX86reg(MipsRegMapHi(m_Opcode.rs),0);
			} else {
				CompConstToVariable(0,&_GPR[m_Opcode.rs].W[1],CRegName::GPR_Hi[m_Opcode.rs]);
			}
			if (m_Section->m_Jump.FallThrough) {
				JgLabel32 (m_Section->m_Cont.BranchLabel.c_str(), 0 );
				m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
				JlLabel8("Continue",0);
				Jump = m_RecompPos - 1;
			} else if (m_Section->m_Cont.FallThrough) {
				JgLabel8("Continue",0);
				Jump = m_RecompPos - 1;
				JlLabel32(m_Section->m_Jump.BranchLabel.c_str(),0);
				m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
			} else {
				JgLabel32 (m_Section->m_Cont.BranchLabel.c_str(), 0 );
				m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
				JlLabel32(m_Section->m_Jump.BranchLabel.c_str(),0);
				m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
			}

			if (IsMapped(m_Opcode.rs)) {
				CompConstToX86reg(cMipsRegMapLo(m_Opcode.rs),0);
			} else {
				CompConstToVariable(0,&_GPR[m_Opcode.rs].W[0],CRegName::GPR_Lo[m_Opcode.rs]);
			}
			if (m_Section->m_Jump.FallThrough) {
				JneLabel32 (m_Section->m_Cont.BranchLabel.c_str(), 0 );
				m_Section->m_Cont.LinkLocation2 = (DWORD *)(m_RecompPos - 4);
				CPU_Message("      continue:");
				*((BYTE *)(Jump))=(BYTE)(m_RecompPos - Jump - 1);
			} else if (m_Section->m_Cont.FallThrough) {
				JeLabel32 (m_Section->m_Jump.BranchLabel.c_str(), 0 );
				m_Section->m_Jump.LinkLocation2 = (DWORD *)(m_RecompPos - 4);
				CPU_Message("      continue:");
				*((BYTE *)(Jump))=(BYTE)(m_RecompPos - Jump - 1);
			} else {
				JneLabel32 (m_Section->m_Cont.BranchLabel.c_str(), 0 );
				m_Section->m_Cont.LinkLocation2 = (DWORD *)(m_RecompPos - 4);
				JmpLabel32("BranchToJump",0);
				m_Section->m_Jump.LinkLocation2 = (DWORD *)(m_RecompPos - 4);
			}
		}
	}
}

void CRecompilerOps::BLTZ_Compare (void) {
	if (IsConst(m_Opcode.rs)) {
		if (Is64Bit(m_Opcode.rs)) {
			if (m_Section->MipsReg_S(m_Opcode.rs) < 0) {
				m_Section->m_Jump.FallThrough = TRUE;
				m_Section->m_Cont.FallThrough = FALSE;
			} else {
				m_Section->m_Jump.FallThrough = FALSE;
				m_Section->m_Cont.FallThrough = TRUE;
			}
		} else if (m_Section->IsSigned(m_Opcode.rs)) {
			if (m_Section->MipsRegLo_S(m_Opcode.rs) < 0) {
				m_Section->m_Jump.FallThrough = TRUE;
				m_Section->m_Cont.FallThrough = FALSE;
			} else {
				m_Section->m_Jump.FallThrough = FALSE;
				m_Section->m_Cont.FallThrough = TRUE;
			}
		} else {
			m_Section->m_Jump.FallThrough = FALSE;
			m_Section->m_Cont.FallThrough = TRUE;
		}
	} else if (IsMapped(m_Opcode.rs)) {
		if (Is64Bit(m_Opcode.rs)) {
			CompConstToX86reg(MipsRegMapHi(m_Opcode.rs),0);
			if (m_Section->m_Jump.FallThrough) {
				JgeLabel32 (m_Section->m_Cont.BranchLabel.c_str(), 0 );
				m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
			} else if (m_Section->m_Cont.FallThrough) {
				JlLabel32(m_Section->m_Jump.BranchLabel.c_str(),0);
				m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
			} else {
				JgeLabel32 (m_Section->m_Cont.BranchLabel.c_str(), 0 );
				m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
				JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(),0);
				m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
			}
		} else if (m_Section->IsSigned(m_Opcode.rs)) {
			CompConstToX86reg(cMipsRegMapLo(m_Opcode.rs),0);
			if (m_Section->m_Jump.FallThrough) {
				JgeLabel32 (m_Section->m_Cont.BranchLabel.c_str(), 0 );
				m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
			} else if (m_Section->m_Cont.FallThrough) {
				JlLabel32(m_Section->m_Jump.BranchLabel.c_str(),0);
				m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
			} else {
				JgeLabel32 (m_Section->m_Cont.BranchLabel.c_str(), 0 );
				m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
				JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(),0);
				m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
			}
		} else {
			m_Section->m_Jump.FallThrough = FALSE;
			m_Section->m_Cont.FallThrough = TRUE;
		}
	} else if (m_Section->IsUnknown(m_Opcode.rs)) {
		CompConstToVariable(0,&_GPR[m_Opcode.rs].W[1],CRegName::GPR_Hi[m_Opcode.rs]);
		if (m_Section->m_Jump.FallThrough) {
			JgeLabel32 (m_Section->m_Cont.BranchLabel.c_str(), 0 );
			m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
		} else if (m_Section->m_Cont.FallThrough) {
			JlLabel32(m_Section->m_Jump.BranchLabel.c_str(),0);
			m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
		} else {
			JlLabel32(m_Section->m_Jump.BranchLabel.c_str(),0);
			m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
			JmpLabel32 (m_Section->m_Cont.BranchLabel.c_str(), 0 );
			m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
		}
	}
}

void CRecompilerOps::BGEZ_Compare (void) {
	if (IsConst(m_Opcode.rs)) {
		if (Is64Bit(m_Opcode.rs)) {
#ifndef EXTERNAL_RELEASE
			DisplayError("BGEZ 1");
#endif
			CRecompilerOps::UnknownOpcode();
		} else if (m_Section->IsSigned(m_Opcode.rs)) {
			if (m_Section->MipsRegLo_S(m_Opcode.rs) >= 0) {
				m_Section->m_Jump.FallThrough = TRUE;
				m_Section->m_Cont.FallThrough = FALSE;
			} else {
				m_Section->m_Jump.FallThrough = FALSE;
				m_Section->m_Cont.FallThrough = TRUE;
			}
		} else {
			m_Section->m_Jump.FallThrough = TRUE;
			m_Section->m_Cont.FallThrough = FALSE;
		}
	} else if (IsMapped(m_Opcode.rs)) {
		if (Is64Bit(m_Opcode.rs)) { 
			CompConstToX86reg(MipsRegMapHi(m_Opcode.rs),0);
			if (m_Section->m_Cont.FallThrough) {
				JgeLabel32 ( m_Section->m_Jump.BranchLabel.c_str(), 0 );
				m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
			} else if (m_Section->m_Jump.FallThrough) {
				JlLabel32 ( m_Section->m_Cont.BranchLabel.c_str(), 0 );
				m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
			} else {
				JlLabel32 ( m_Section->m_Cont.BranchLabel.c_str(), 0 );
				m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
				JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(),0);
				m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
			}
		} else if (m_Section->IsSigned(m_Opcode.rs)) {
			CompConstToX86reg(cMipsRegMapLo(m_Opcode.rs),0);
			if (m_Section->m_Cont.FallThrough) {
				JgeLabel32 ( m_Section->m_Jump.BranchLabel.c_str(), 0 );
				m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
			} else if (m_Section->m_Jump.FallThrough) {
				JlLabel32 ( m_Section->m_Cont.BranchLabel.c_str(), 0 );
				m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
			} else {
				JlLabel32 ( m_Section->m_Cont.BranchLabel.c_str(), 0 );
				m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
				JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(),0);
				m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
			}
		} else { 
			m_Section->m_Jump.FallThrough = TRUE;
			m_Section->m_Cont.FallThrough = FALSE;
		}
	} else {
		CompConstToVariable(0,&_GPR[m_Opcode.rs].W[1],CRegName::GPR_Hi[m_Opcode.rs]);		
		if (m_Section->m_Cont.FallThrough) {
			JgeLabel32 ( m_Section->m_Jump.BranchLabel.c_str(), 0 );
			m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
		} else if (m_Section->m_Jump.FallThrough) {
			JlLabel32 ( m_Section->m_Cont.BranchLabel.c_str(), 0 );
			m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
		} else {
			JlLabel32 ( m_Section->m_Cont.BranchLabel.c_str(), 0 );
			m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
			JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(),0);
			m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
		}
	}
}

void CRecompilerOps::COP1_BCF_Compare (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	TestVariable(FPCSR_C,&_FPCR[31],"_FPCR[31]");
	if (m_Section->m_Cont.FallThrough) {
		JeLabel32 ( m_Section->m_Jump.BranchLabel.c_str(), 0 );
		m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
	} else if (m_Section->m_Jump.FallThrough) {
		JneLabel32 ( m_Section->m_Cont.BranchLabel.c_str(), 0 );
		m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
	} else {
		JneLabel32 ( m_Section->m_Cont.BranchLabel.c_str(), 0 );
		m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
		JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(),0);
		m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
	}
#endif
}

void CRecompilerOps::COP1_BCT_Compare (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	TestVariable(FPCSR_C,&_FPCR[31],"_FPCR[31]");
	if (m_Section->m_Cont.FallThrough) {
		JneLabel32 ( m_Section->m_Jump.BranchLabel.c_str(), 0 );
		m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
	} else if (m_Section->m_Jump.FallThrough) {
		JeLabel32 ( m_Section->m_Cont.BranchLabel.c_str(), 0 );
		m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
	} else {
		JeLabel32 ( m_Section->m_Cont.BranchLabel.c_str(), 0 );
		m_Section->m_Cont.LinkLocation = (DWORD *)(m_RecompPos - 4);
		JmpLabel32(m_Section->m_Jump.BranchLabel.c_str(),0);
		m_Section->m_Jump.LinkLocation = (DWORD *)(m_RecompPos - 4);
	}
#endif
}

/*************************  OpCode functions *************************/
void CRecompilerOps::J (void) {
	if ( m_NextInstruction == NORMAL ) {
		CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

		m_Section->m_Jump.TargetPC      = (m_CompilePC & 0xF0000000) + (m_Opcode.target << 2);;
		if (m_Section->m_JumpSection != NULL) {
			m_Section->m_Jump.BranchLabel.Format("Section_%d",((CCodeSection *)m_Section->m_JumpSection)->m_SectionID);
		} else {
			m_Section->m_Jump.BranchLabel = "ExitBlock";
		}
		m_Section->m_Jump.FallThrough   = TRUE;
		m_Section->m_Jump.LinkLocation  = NULL;
		m_Section->m_Jump.LinkLocation2 = NULL;
		m_NextInstruction = DO_DELAY_SLOT;
		if ((m_CompilePC & 0xFFC) == 0xFFC) {
			memcpy(&m_Section->m_Jump.RegSet,&m_RegWorkingSet,sizeof(CRegInfo));
			m_Section->GenerateSectionLinkage();
			m_NextInstruction = END_BLOCK;
		}
	} else if (m_NextInstruction == DELAY_SLOT_DONE ) {		
		memcpy(&m_Section->m_Jump.RegSet,&m_RegWorkingSet,sizeof(CRegInfo));
		m_Section->GenerateSectionLinkage();
		m_NextInstruction = END_BLOCK;
	} else {
#ifndef EXTERNAL_RELEASE
		DisplayError("WTF\n\nJ\nNextInstruction = %X", m_NextInstruction);
#endif
	}
}

void CRecompilerOps::JAL (void) {
	if ( m_NextInstruction == NORMAL ) {
		CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
		UnMap_GPR( 31, FALSE);
		MipsRegLo(31) = m_CompilePC + 8;
		MipsRegState(31) = CRegInfo::STATE_CONST_32;
		if ((m_CompilePC & 0xFFC) == 0xFFC) {
			_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
			MoveConstToVariable((m_CompilePC & 0xF0000000) + (m_Opcode.target << 2),&JumpToLocation,"JumpToLocation");
			MoveConstToVariable(m_CompilePC + 4,_PROGRAM_COUNTER,"_PROGRAM_COUNTER");
			UpdateCounters(&m_RegWorkingSet.BlockCycleCount(),&m_RegWorkingSet.BlockRandomModifier(),FALSE);
			m_RegWorkingSet.WriteBackRegisters();
			if (CPU_Type == CPU_SyncCores) { Call_Direct(SyncToPC, "SyncToPC"); }
			MoveConstToVariable(DELAY_SLOT,&m_NextInstruction,"m_NextInstruction");
			Ret();
			m_NextInstruction = END_BLOCK;
			return;
#endif
		}
		m_NextInstruction = DO_DELAY_SLOT;
	} else if (m_NextInstruction == DELAY_SLOT_DONE ) {		
		DWORD TargetPC = (m_CompilePC & 0xF0000000) + (m_Opcode.target << 2);
		m_Section->CompileExit(m_CompilePC,TargetPC,m_RegWorkingSet,CExitInfo::Normal,TRUE,NULL);
		m_NextInstruction = END_BLOCK;
	} else {
#ifndef EXTERNAL_RELEASE
		DisplayError("WTF\n\nBranch\nNextInstruction = %X", m_NextInstruction);
#endif
	}
	return;
}

void CRecompilerOps::ADDI (void) {
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	if (m_Opcode.rt == 0) { return; }

#ifdef tofix
	if (SPHack && m_Opcode.rs == 29 && m_Opcode.rt == 29) {
		AddConstToX86Reg(Map_MemoryStack(m_Section, x86_Any, true),(short)m_Opcode.immediate);
	}
#endif

	if (IsConst(m_Opcode.rs)) { 
		if (IsMapped(m_Opcode.rt)) { UnMap_GPR(m_Opcode.rt, FALSE); }
		MipsRegLo(m_Opcode.rt) = cMipsRegLo(m_Opcode.rs) + (short)m_Opcode.immediate;
		MipsRegState(m_Opcode.rt) = CRegInfo::STATE_CONST_32;
	} else {
		Map_GPR_32bit(m_Opcode.rt,TRUE,m_Opcode.rs);
		AddConstToX86Reg(cMipsRegMapLo(m_Opcode.rt),(short)m_Opcode.immediate);
	}
#ifdef tofix
	if (SPHack && m_Opcode.rt == 29 && m_Opcode.rs != 29) { 
		ResetX86Protection();
		_MMU->ResetMemoryStack(m_Section); 
	}
#endif
}

void CRecompilerOps::ADDIU (void) {
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	if (m_Opcode.rt == 0 || (m_Opcode.immediate == 0 && m_Opcode.rs == m_Opcode.rt)) { return; }

#ifdef tofix
	if (SPHack)
	{
		if (m_Opcode.rs == 29 && m_Opcode.rt == 29) 
		{
			AddConstToX86Reg(Map_MemoryStack(m_Section, x86_Any, TRUE),(short)m_Opcode.immediate);
		}
	}
#endif

	if (IsConst(m_Opcode.rs)) { 
		if (IsMapped(m_Opcode.rt)) { UnMap_GPR(m_Opcode.rt, FALSE); }
		MipsRegLo(m_Opcode.rt) = cMipsRegLo(m_Opcode.rs) + (short)m_Opcode.immediate;
		MipsRegState(m_Opcode.rt) = CRegInfo::STATE_CONST_32;
	} else {
		Map_GPR_32bit(m_Opcode.rt,TRUE,m_Opcode.rs);
		AddConstToX86Reg(cMipsRegMapLo(m_Opcode.rt),(short)m_Opcode.immediate);
	}

#ifdef tofix
	if (SPHack && m_Opcode.rt == 29 && m_Opcode.rs != 29) { 
		ResetX86Protection();
		_MMU->ResetMemoryStack(m_Section); 
	}
#endif
}

void CRecompilerOps::SLTIU (void) {
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rt == 0) { return; }

	if (IsConst(m_Opcode.rs)) { 
		DWORD Result;

		if (Is64Bit(m_Opcode.rs)) {
			__int64 Immediate = (__int64)((short)m_Opcode.immediate);
			Result = m_Section->MipsReg(m_Opcode.rs) < ((unsigned)(Immediate))?1:0;
		} else if (m_Section->Is32Bit(m_Opcode.rs)) {
			Result = cMipsRegLo(m_Opcode.rs) < ((unsigned)((short)m_Opcode.immediate))?1:0;
		}
		UnMap_GPR(m_Opcode.rt, FALSE);
		MipsRegState(m_Opcode.rt) = CRegInfo::STATE_CONST_32;
		MipsRegLo(m_Opcode.rt) = Result;
	} else if (IsMapped(m_Opcode.rs)) { 
		if (Is64Bit(m_Opcode.rs)) {
			BYTE * Jump[2];

			CompConstToX86reg(cMipsRegMapHi(m_Opcode.rs),((short)m_Opcode.immediate >> 31));
			JeLabel8("Low Compare",0);
			Jump[0] = m_RecompPos - 1;
			SetbVariable(&m_BranchCompare,"m_BranchCompare");
			JmpLabel8("Continue",0);
			Jump[1] = m_RecompPos - 1;
			CPU_Message("");
			CPU_Message("      Low Compare:");
			*((BYTE *)(Jump[0]))=(BYTE)(m_RecompPos - Jump[0] - 1);
			CompConstToX86reg(cMipsRegMapLo(m_Opcode.rs),(short)m_Opcode.immediate);
			SetbVariable(&m_BranchCompare,"m_BranchCompare");
			CPU_Message("");
			CPU_Message("      Continue:");
			*((BYTE *)(Jump[1]))=(BYTE)(m_RecompPos - Jump[1] - 1);
			Map_GPR_32bit(m_Opcode.rt,FALSE, -1);
			MoveVariableToX86reg(&m_BranchCompare,"m_BranchCompare",cMipsRegMapLo(m_Opcode.rt));
		} else {
			CompConstToX86reg(cMipsRegMapLo(m_Opcode.rs),(short)m_Opcode.immediate);
			SetbVariable(&m_BranchCompare,"m_BranchCompare");
			Map_GPR_32bit(m_Opcode.rt,FALSE, -1);
			MoveVariableToX86reg(&m_BranchCompare,"m_BranchCompare",cMipsRegMapLo(m_Opcode.rt));
		}
	} else {
		BYTE * Jump;

		CompConstToVariable(((short)m_Opcode.immediate >> 31),&_GPR[m_Opcode.rs].W[1],CRegName::GPR_Hi[m_Opcode.rs]);
		JneLabel8("CompareSet",0);
		Jump = m_RecompPos - 1;
		CompConstToVariable((short)m_Opcode.immediate,&_GPR[m_Opcode.rs].W[0],CRegName::GPR_Lo[m_Opcode.rs]);
		CPU_Message("");
		CPU_Message("      CompareSet:");
		*((BYTE *)(Jump))=(BYTE)(m_RecompPos - Jump - 1);
		SetbVariable(&m_BranchCompare,"m_BranchCompare");
		Map_GPR_32bit(m_Opcode.rt,FALSE, -1);
		MoveVariableToX86reg(&m_BranchCompare,"m_BranchCompare",cMipsRegMapLo(m_Opcode.rt));
		
		
		/*SetbVariable(&m_BranchCompare,"m_BranchCompare");
		JmpLabel8("Continue",0);
		Jump[1] = m_RecompPos - 1;
		CPU_Message("");
		CPU_Message("      Low Compare:");
		*((BYTE *)(Jump[0]))=(BYTE)(m_RecompPos - Jump[0] - 1);
		CompConstToVariable((short)m_Opcode.immediate,&_GPR[m_Opcode.rs].W[0],CRegName::GPR_Lo[m_Opcode.rs]);
		SetbVariable(&m_BranchCompare,"m_BranchCompare");
		CPU_Message("");
		CPU_Message("      Continue:");
		*((BYTE *)(Jump[1]))=(BYTE)(m_RecompPos - Jump[1] - 1);
		Map_GPR_32bit(m_Opcode.rt,FALSE, -1);
		MoveVariableToX86reg(&m_BranchCompare,"m_BranchCompare",cMipsRegMapLo(m_Opcode.rt));*/
	}
}

void CRecompilerOps::SLTI (void) 
{
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rt == 0) { return; }

	if (IsConst(m_Opcode.rs)) { 
		DWORD Result;

		if (Is64Bit(m_Opcode.rs)) {
			__int64 Immediate = (__int64)((short)m_Opcode.immediate);
			Result = (__int64)m_Section->MipsReg(m_Opcode.rs) < Immediate?1:0;
		} else if (m_Section->Is32Bit(m_Opcode.rs)) {
			Result = m_Section->MipsRegLo_S(m_Opcode.rs) < (short)m_Opcode.immediate?1:0;
		}
		UnMap_GPR(m_Opcode.rt, FALSE);
		MipsRegState(m_Opcode.rt) = CRegInfo::STATE_CONST_32;
		MipsRegLo(m_Opcode.rt) = Result;
	} else if (IsMapped(m_Opcode.rs)) { 
		if (Is64Bit(m_Opcode.rs)) {
			BYTE * Jump[2];

			CompConstToX86reg(MipsRegMapHi(m_Opcode.rs),((short)m_Opcode.immediate >> 31));
			JeLabel8("Low Compare",0);
			Jump[0] = m_RecompPos - 1;
			SetlVariable(&m_BranchCompare,"m_BranchCompare");
			JmpLabel8("Continue",0);
			Jump[1] = m_RecompPos - 1;
			CPU_Message("");
			CPU_Message("      Low Compare:");
			*((BYTE *)(Jump[0]))=(BYTE)(m_RecompPos - Jump[0] - 1);
			CompConstToX86reg(cMipsRegMapLo(m_Opcode.rs),(short)m_Opcode.immediate);
			SetbVariable(&m_BranchCompare,"m_BranchCompare");
			CPU_Message("");
			CPU_Message("      Continue:");
			*((BYTE *)(Jump[1]))=(BYTE)(m_RecompPos - Jump[1] - 1);
			Map_GPR_32bit(m_Opcode.rt,FALSE, -1);
			MoveVariableToX86reg(&m_BranchCompare,"m_BranchCompare",cMipsRegMapLo(m_Opcode.rt));
		} else {
		/*	CompConstToX86reg(cMipsRegMapLo(m_Opcode.rs),(short)m_Opcode.immediate);
			SetlVariable(&m_BranchCompare,"m_BranchCompare");
			Map_GPR_32bit(m_Opcode.rt,FALSE, -1);
			MoveVariableToX86reg(&m_BranchCompare,"m_BranchCompare",cMipsRegMapLo(m_Opcode.rt));
			*/
			ProtectGPR( m_Opcode.rs);
			Map_GPR_32bit(m_Opcode.rt,FALSE, -1);
			CompConstToX86reg(cMipsRegMapLo(m_Opcode.rs),(short)m_Opcode.immediate);
			
			if (cMipsRegMapLo(m_Opcode.rt) > x86_EBX) {
				SetlVariable(&m_BranchCompare,"m_BranchCompare");
				MoveVariableToX86reg(&m_BranchCompare,"m_BranchCompare",cMipsRegMapLo(m_Opcode.rt));
			} else {
				Setl(cMipsRegMapLo(m_Opcode.rt));
				AndConstToX86Reg(cMipsRegMapLo(m_Opcode.rt), 1);
			}
		}
	} else {
		BYTE * Jump[2];

		CompConstToVariable(((short)m_Opcode.immediate >> 31),&_GPR[m_Opcode.rs].W[1],CRegName::GPR_Hi[m_Opcode.rs]);
		JeLabel8("Low Compare",0);
		Jump[0] = m_RecompPos - 1;
		SetlVariable(&m_BranchCompare,"m_BranchCompare");
		JmpLabel8("Continue",0);
		Jump[1] = m_RecompPos - 1;
		CPU_Message("");
		CPU_Message("      Low Compare:");
		*((BYTE *)(Jump[0]))=(BYTE)(m_RecompPos - Jump[0] - 1);
		CompConstToVariable((short)m_Opcode.immediate,&_GPR[m_Opcode.rs].W[0],CRegName::GPR_Lo[m_Opcode.rs]);
		SetbVariable(&m_BranchCompare,"m_BranchCompare");
		CPU_Message("");
		CPU_Message("      Continue:");
		*((BYTE *)(Jump[1]))=(BYTE)(m_RecompPos - Jump[1] - 1);
		Map_GPR_32bit(m_Opcode.rt,FALSE, -1);
		MoveVariableToX86reg(&m_BranchCompare,"m_BranchCompare",cMipsRegMapLo(m_Opcode.rt));
	}
}

void CRecompilerOps::ANDI (void) {
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	if (m_Opcode.rt == 0) { return;}

	if (IsConst(m_Opcode.rs)) {
		if (IsMapped(m_Opcode.rt)) { UnMap_GPR(m_Opcode.rt, FALSE); }
		MipsRegState(m_Opcode.rt) = CRegInfo::STATE_CONST_32;
		MipsRegLo(m_Opcode.rt) = cMipsRegLo(m_Opcode.rs) & m_Opcode.immediate;
	} else if (m_Opcode.immediate != 0) { 
		Map_GPR_32bit(m_Opcode.rt,FALSE,m_Opcode.rs);
		AndConstToX86Reg(cMipsRegMapLo(m_Opcode.rt),m_Opcode.immediate);
	} else {
		Map_GPR_32bit(m_Opcode.rt,FALSE,0);
	}
}

void CRecompilerOps::ORI (void) {
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rt == 0) { return;}

	if (IsConst(m_Opcode.rs)) {
		if (IsMapped(m_Opcode.rt)) { UnMap_GPR(m_Opcode.rt, FALSE); }
		MipsRegState(m_Opcode.rt) = MipsRegState(m_Opcode.rs);
		MipsRegHi(m_Opcode.rt) = MipsRegHi(m_Opcode.rs);
		MipsRegLo(m_Opcode.rt) = cMipsRegLo(m_Opcode.rs) | m_Opcode.immediate;
	} else if (IsMapped(m_Opcode.rs)) {
		if (Is64Bit(m_Opcode.rs)) {
			Map_GPR_64bit(m_Opcode.rt,m_Opcode.rs);
		} else {
			Map_GPR_32bit(m_Opcode.rt,m_Section->IsSigned(m_Opcode.rs),m_Opcode.rs);
		}
		OrConstToX86Reg(m_Opcode.immediate,cMipsRegMapLo(m_Opcode.rt));
	} else {
		Map_GPR_64bit(m_Opcode.rt,m_Opcode.rs);
		OrConstToX86Reg(m_Opcode.immediate,cMipsRegMapLo(m_Opcode.rt));
	}
}

void CRecompilerOps::XORI (void) {
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rt == 0) { return;}

	if (IsConst(m_Opcode.rs)) {
		if (m_Opcode.rs != m_Opcode.rt) { UnMap_GPR(m_Opcode.rt, FALSE); }
		MipsRegState(m_Opcode.rt) = MipsRegState(m_Opcode.rs);
		MipsRegHi(m_Opcode.rt) = MipsRegHi(m_Opcode.rs);
		MipsRegLo(m_Opcode.rt) = cMipsRegLo(m_Opcode.rs) ^ m_Opcode.immediate;
	} else {
		if (IsMapped(m_Opcode.rs) && m_Section->Is32Bit(m_Opcode.rs)) {
			Map_GPR_32bit(m_Opcode.rt,m_Section->IsSigned(m_Opcode.rs),m_Opcode.rs);
		} else {
			Map_GPR_64bit(m_Opcode.rt,m_Opcode.rs);
		}
		if (m_Opcode.immediate != 0) { XorConstToX86Reg(cMipsRegMapLo(m_Opcode.rt),m_Opcode.immediate); }
	}
}

void CRecompilerOps::LUI (void) {
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rt == 0) { return;}

#ifdef tofix
	if (SPHack && m_Opcode.rt == 29) {
		x86Reg Reg = Map_MemoryStack(m_Section, x86_Any, false);
		DWORD Address;

		TranslateVaddr (((short)m_Opcode.offset << 16), &Address);
		if (x86reg < 0) {
			MoveConstToVariable((DWORD)(Address + RDRAM), g_MemoryStack, "MemoryStack");
		} else {
			MoveConstToX86reg((DWORD)(Address + RDRAM), x86reg);
		}
	}
#endif
	UnMap_GPR(m_Opcode.rt, FALSE);
	MipsRegLo(m_Opcode.rt) = ((short)m_Opcode.offset << 16);
	MipsRegState(m_Opcode.rt) = CRegInfo::STATE_CONST_32;
}

void CRecompilerOps::DADDIU (void) {
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	if (m_Opcode.rs != 0) { UnMap_GPR(m_Opcode.rs,TRUE); }
	if (m_Opcode.rs != 0) { UnMap_GPR(m_Opcode.rt,TRUE); }
	Pushad();
	MoveConstToVariable(m_Opcode.Hex, &R4300iOp::m_Opcode.Hex, "R4300iOp::m_Opcode.Hex");
	Call_Direct(R4300iOp::DADDIU, "R4300iOp::DADDIU");
	Popad();
}

void CRecompilerOps::CACHE (void){
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	if (_Settings->LoadDword(Game_SMM_Cache) == 0)
	{
		return;
	}

	switch(m_Opcode.rt) {
	case 0:
	case 16:
		Pushad();
		PushImm32("CRecompiler::Remove_Cache",CRecompiler::Remove_Cache);
		PushImm32("20",20);
		if (IsConst(m_Opcode.base)) { 
			DWORD Address = cMipsRegLo(m_Opcode.base) + (short)m_Opcode.offset;
			PushImm32("Address",Address);
		} else if (IsMapped(m_Opcode.base)) { 
			AddConstToX86Reg(cMipsRegMapLo(m_Opcode.base),(short)m_Opcode.offset);
			Push(cMipsRegMapLo(m_Opcode.base));
		} else {
			MoveVariableToX86reg(&_GPR[m_Opcode.base].UW[0],CRegName::GPR_Lo[m_Opcode.base],x86_EAX);
			AddConstToX86Reg(x86_EAX,(short)m_Opcode.offset);
			Push(x86_EAX);
		}
		MoveConstToX86reg((DWORD)_N64System->GetRecompiler(),x86_ECX);		
		Call_Direct(AddressOf(&CRecompiler::ClearRecompCode_Virt), "CRecompiler::ClearRecompCode_Virt");
		Popad();
		break;
	case 1:
	case 3:
	case 13:
	case 5:
	case 8:
	case 9:
	case 17:
	case 21:
	case 25:
		break;
#ifndef EXTERNAL_RELEASE
	default:
		DisplayError("cache: %d",m_Opcode.rt);
#endif
	}
}

void CRecompilerOps::LL (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	x86Reg TempReg1, TempReg2;

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	if (m_Opcode.rt == 0) return;

	if (IsConst(m_Opcode.base)) { 
		DWORD Address = cMipsRegLo(m_Opcode.base) + (short)m_Opcode.offset;
		Map_GPR_32bit(m_Opcode.rt,TRUE,-1);
		_MMU->Compile_LW(m_Section, cMipsRegLo(m_Opcode.rt),Address);
		MoveConstToVariable(1,_LLBit,"LLBit");
		
		_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
		TranslateVaddr(Address, &Address);
#endif
		MoveConstToVariable(Address,_LLAddr,"LLAddr");
		return;
	}
	if (g_UseTlb) {	
		if (IsMapped(m_Opcode.rt)) { ProtectGPR(m_Opcode.rt); }
		if (IsMapped(m_Opcode.base) && m_Opcode.offset == 0) { 
			ProtectGPR(m_Opcode.base);
			TempReg1 = cMipsRegLo(m_Opcode.base);
		} else {
			if (IsMapped(m_Opcode.base)) { 
				ProtectGPR(m_Opcode.base);
				if (m_Opcode.offset != 0) {
					TempReg1 = Map_TempReg(x86_Any,-1,FALSE);
					LeaSourceAndOffset(TempReg1,cMipsRegLo(m_Opcode.base),(short)m_Opcode.offset);
				} else {
					TempReg1 = Map_TempReg(x86_Any,m_Opcode.base,FALSE);
				}
			} else {
				TempReg1 = Map_TempReg(x86_Any,m_Opcode.base,FALSE);
				AddConstToX86Reg(TempReg1,(short)m_Opcode.immediate);
			}
		}
		TempReg2 = Map_TempReg(x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
		CompileReadTLBMiss(m_Section,TempReg1,TempReg2);
		Map_GPR_32bit(m_Opcode.rt,TRUE,-1);
		MoveX86regPointerToX86reg(TempReg1, TempReg2,cMipsRegLo(m_Opcode.rt));
		MoveConstToVariable(1,_LLBit,"LLBit");
		MoveX86regToVariable(TempReg1,_LLAddr,"LLAddr");
		AddX86regToVariable(TempReg2,_LLAddr,"LLAddr");
		SubConstFromVariable((DWORD)_MMU->Rdram(),_LLAddr,"LLAddr");
	} else {
		if (IsMapped(m_Opcode.base)) { 
			ProtectGPR(m_Opcode.base);
			if (m_Opcode.offset != 0) {
				Map_GPR_32bit(m_Opcode.rt,TRUE,-1);
				LeaSourceAndOffset(cMipsRegLo(m_Opcode.rt),cMipsRegLo(m_Opcode.base),(short)m_Opcode.offset);
			} else {
				Map_GPR_32bit(m_Opcode.rt,TRUE,m_Opcode.base);
			}
		} else {
			Map_GPR_32bit(m_Opcode.rt,TRUE,m_Opcode.base);
			AddConstToX86Reg(cMipsRegMapLo(m_Opcode.rt),(short)m_Opcode.immediate);
		}
		AndConstToX86Reg(cMipsRegLo(m_Opcode.rt),0x1FFFFFFF);
		MoveX86regToVariable(cMipsRegLo(m_Opcode.rt),_LLAddr,"LLAddr");
		MoveN64MemToX86reg(cMipsRegLo(m_Opcode.rt),cMipsRegLo(m_Opcode.rt));
		MoveConstToVariable(1,_LLBit,"LLBit");
	}
#endif
}

void CRecompilerOps::SC (void){
#ifdef tofix
	x86Reg TempReg1, TempReg2;
	BYTE * Jump;

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	
	CompConstToVariable(1,_LLBit,"LLBit");
	JneLabel32("LLBitNotSet",0);
	Jump = (DWORD *)(m_RecompPos - 4);
	if (IsConst(m_Opcode.base)) { 
		DWORD Address = cMipsRegLo(m_Opcode.base) + (short)m_Opcode.offset;
			
		if (IsConst(m_Opcode.rt)) {
			_MMU->Compile_SW_Const(cMipsRegLo(m_Opcode.rt), Address);
		} else if (IsMapped(m_Opcode.rt)) {
			_MMU->Compile_SW_Register(m_Section,cMipsRegLo(m_Opcode.rt), Address);
		} else {
			_MMU->Compile_SW_Register(m_Section,Map_TempReg(x86_Any,m_Opcode.rt,FALSE), Address);
		}
		CPU_Message("      LLBitNotSet:");
		*((DWORD *)(Jump))=(BYTE)(m_RecompPos - Jump - 4);
		Map_GPR_32bit(m_Opcode.rt,FALSE,-1);
		MoveVariableToX86reg(_LLBit,"LLBit",cMipsRegLo(m_Opcode.rt));
		return;
	}
	if (IsMapped(m_Opcode.rt)) { ProtectGPR(m_Opcode.rt); }
	if (IsMapped(m_Opcode.base)) { 
		ProtectGPR(m_Opcode.base);
		if (m_Opcode.offset != 0) {
			TempReg1 = Map_TempReg(x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,cMipsRegLo(m_Opcode.base),(short)m_Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(x86_Any,m_Opcode.base,FALSE);
		}
		UnProtectGPR(m_Opcode.base);
	} else {
		TempReg1 = Map_TempReg(x86_Any,m_Opcode.base,FALSE);
		AddConstToX86Reg(TempReg1,(short)m_Opcode.immediate);
	}
	if (g_UseTlb) {
		TempReg2 = Map_TempReg(x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_WriteMap,"TLB_WriteMap",TempReg2,TempReg2,4);
		//For tlb miss
		//0041C522 85 C0                test        eax,eax
		//0041C524 75 01                jne         0041C527

		if (IsConst(m_Opcode.rt)) {
			MoveConstToX86regPointer(cMipsRegLo(m_Opcode.rt),TempReg1, TempReg2);
		} else if (IsMapped(m_Opcode.rt)) {
			MoveX86regToX86regPointer(cMipsRegLo(m_Opcode.rt),TempReg1, TempReg2);
		} else {	
			MoveX86regToX86regPointer(Map_TempReg(x86_Any,m_Opcode.rt,FALSE),TempReg1, TempReg2);
		}
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		if (IsConst(m_Opcode.rt)) {
			MoveConstToN64Mem(cMipsRegLo(m_Opcode.rt),TempReg1);
		} else if (IsMapped(m_Opcode.rt)) {
			MoveX86regToN64Mem(cMipsRegLo(m_Opcode.rt),TempReg1);
		} else {	
			MoveX86regToN64Mem(Map_TempReg(x86_Any,m_Opcode.rt,FALSE),TempReg1);
		}
	}
	CPU_Message("      LLBitNotSet:");
	*((DWORD *)(Jump))=(BYTE)(m_RecompPos - Jump - 4);
	Map_GPR_32bit(m_Opcode.rt,FALSE,-1);
	MoveVariableToX86reg(_LLBit,"LLBit",cMipsRegLo(m_Opcode.rt));
#endif
}

/********************** R4300i OpCodes: Special **********************/
void CRecompilerOps::SPECIAL_SLL (void) {
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	if (m_Opcode.rd == 0) { return; }
	if (IsConst(m_Opcode.rt)) {
		if (IsMapped(m_Opcode.rd)) { UnMap_GPR(m_Opcode.rd, FALSE); }
		MipsRegLo(m_Opcode.rd) = cMipsRegLo(m_Opcode.rt) << m_Opcode.sa;
		MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		return;
	}
	if (m_Opcode.rd != m_Opcode.rt && IsMapped(m_Opcode.rt)) {
		switch (m_Opcode.sa) {
		case 0: 
			Map_GPR_32bit(m_Opcode.rd,TRUE,m_Opcode.rt);
			break;
		case 1:
			ProtectGPR(m_Opcode.rt);
			Map_GPR_32bit(m_Opcode.rd,TRUE,-1);
			LeaRegReg(cMipsRegMapLo(m_Opcode.rd),cMipsRegMapLo(m_Opcode.rt), 2);
			break;			
		case 2:
			ProtectGPR(m_Opcode.rt);
			Map_GPR_32bit(m_Opcode.rd,TRUE,-1);
			LeaRegReg(cMipsRegMapLo(m_Opcode.rd),cMipsRegMapLo(m_Opcode.rt), 4);
			break;			
		case 3:
			ProtectGPR(m_Opcode.rt);
			Map_GPR_32bit(m_Opcode.rd,TRUE,-1);
			LeaRegReg(cMipsRegMapLo(m_Opcode.rd),cMipsRegMapLo(m_Opcode.rt), 8);
			break;
		default:
			Map_GPR_32bit(m_Opcode.rd,TRUE,m_Opcode.rt);
			ShiftLeftSignImmed(cMipsRegMapLo(m_Opcode.rd),(BYTE)m_Opcode.sa);
		}
	} else {
		Map_GPR_32bit(m_Opcode.rd,TRUE,m_Opcode.rt);
		ShiftLeftSignImmed(cMipsRegMapLo(m_Opcode.rd),(BYTE)m_Opcode.sa);
	}
}

void CRecompilerOps::SPECIAL_SRL (void) {
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rd == 0) { return; }
	
	if (IsConst(m_Opcode.rt)) {
		if (IsMapped(m_Opcode.rd)) { UnMap_GPR(m_Opcode.rd, FALSE); }
		MipsRegLo(m_Opcode.rd) = cMipsRegLo(m_Opcode.rt) >> m_Opcode.sa;
		MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		return;
	}
	Map_GPR_32bit(m_Opcode.rd,TRUE,m_Opcode.rt);
	ShiftRightUnsignImmed(cMipsRegMapLo(m_Opcode.rd),(BYTE)m_Opcode.sa);
}

void CRecompilerOps::SPECIAL_SRA (void) {
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rd == 0) { return; }
	
	if (IsConst(m_Opcode.rt)) {
		if (IsMapped(m_Opcode.rd)) { UnMap_GPR(m_Opcode.rd, FALSE); }
		MipsRegLo(m_Opcode.rd) = m_Section->MipsRegLo_S(m_Opcode.rt) >> m_Opcode.sa;
		MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		return;
	}
	Map_GPR_32bit(m_Opcode.rd,TRUE,m_Opcode.rt);
	ShiftRightSignImmed(cMipsRegMapLo(m_Opcode.rd),(BYTE)m_Opcode.sa);
}

void CRecompilerOps::SPECIAL_SLLV (void) {
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rd == 0) { return; }
	
	if (IsConst(m_Opcode.rs)) {
		DWORD Shift = (cMipsRegLo(m_Opcode.rs) & 0x1F);
		if (IsConst(m_Opcode.rt)) {
			if (IsMapped(m_Opcode.rd)) { UnMap_GPR(m_Opcode.rd, FALSE); }
			MipsRegLo(m_Opcode.rd) = cMipsRegLo(m_Opcode.rt) << Shift;
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else {
			Map_GPR_32bit(m_Opcode.rd,TRUE,m_Opcode.rt);
			ShiftLeftSignImmed(cMipsRegMapLo(m_Opcode.rd),(BYTE)Shift);
		}
		return;
	}
	Map_TempReg(x86_ECX,m_Opcode.rs,FALSE);
	AndConstToX86Reg(x86_ECX,0x1F);
	Map_GPR_32bit(m_Opcode.rd,TRUE,m_Opcode.rt);
	ShiftLeftSign(cMipsRegMapLo(m_Opcode.rd));
}

void CRecompilerOps::SPECIAL_SRLV (void) {
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rd == 0) { return; }
	
	if (m_Section->IsKnown(m_Opcode.rs) && IsConst(m_Opcode.rs)) {
		DWORD Shift = (cMipsRegLo(m_Opcode.rs) & 0x1F);
		if (IsConst(m_Opcode.rt)) {
			if (IsMapped(m_Opcode.rd)) { UnMap_GPR(m_Opcode.rd, FALSE); }
			MipsRegLo(m_Opcode.rd) = cMipsRegLo(m_Opcode.rt) >> Shift;
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
			return;
		}
		Map_GPR_32bit(m_Opcode.rd,TRUE,m_Opcode.rt);
		ShiftRightUnsignImmed(cMipsRegMapLo(m_Opcode.rd),(BYTE)Shift);
		return;
	}
	Map_TempReg(x86_ECX,m_Opcode.rs,FALSE);
	AndConstToX86Reg(x86_ECX,0x1F);
	Map_GPR_32bit(m_Opcode.rd,TRUE,m_Opcode.rt);
	ShiftRightUnsign(cMipsRegMapLo(m_Opcode.rd));
}

void CRecompilerOps::SPECIAL_SRAV (void) {
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rd == 0) { return; }
	
	if (m_Section->IsKnown(m_Opcode.rs) && IsConst(m_Opcode.rs)) {
		DWORD Shift = (cMipsRegLo(m_Opcode.rs) & 0x1F);
		if (IsConst(m_Opcode.rt)) {
			if (IsMapped(m_Opcode.rd)) { UnMap_GPR(m_Opcode.rd, FALSE); }
			MipsRegLo(m_Opcode.rd) = m_Section->MipsRegLo_S(m_Opcode.rt) >> Shift;
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
			return;
		}
		Map_GPR_32bit(m_Opcode.rd,TRUE,m_Opcode.rt);
		ShiftRightSignImmed(cMipsRegMapLo(m_Opcode.rd),(BYTE)Shift);
		return;
	}
	Map_TempReg(x86_ECX,m_Opcode.rs,FALSE);
	AndConstToX86Reg(x86_ECX,0x1F);
	Map_GPR_32bit(m_Opcode.rd,TRUE,m_Opcode.rt);
	ShiftRightSign(cMipsRegMapLo(m_Opcode.rd));
}

void CRecompilerOps::SPECIAL_JR (void) {
	if ( m_NextInstruction == NORMAL ) {
		CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
		if (IsConst(m_Opcode.rs)) { 
			m_Section->m_Jump.BranchLabel.Format("0x%08X",cMipsRegLo(m_Opcode.rs));
			m_Section->m_Jump.TargetPC      = cMipsRegLo(m_Opcode.rs);
			m_Section->m_Jump.FallThrough   = TRUE;
			m_Section->m_Jump.LinkLocation  = NULL;
			m_Section->m_Jump.LinkLocation2 = NULL;
			m_Section->m_Cont.FallThrough   = FALSE;
			m_Section->m_Cont.LinkLocation  = NULL;
			m_Section->m_Cont.LinkLocation2 = NULL;
			if ((m_CompilePC & 0xFFC) == 0xFFC) {
				m_Section->GenerateSectionLinkage();
				m_NextInstruction = END_BLOCK;
				return;
			}
		}
		if ((m_CompilePC & 0xFFC) == 0xFFC) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
			if (IsMapped(m_Opcode.rs)) { 
				MoveX86regToVariable(cMipsRegLo(m_Opcode.rs),&JumpToLocation, "JumpToLocation");
			} else {
				MoveX86regToVariable(Map_TempReg(x86_Any,m_Opcode.rs,FALSE),&JumpToLocation, "JumpToLocation");
			}
			MoveConstToVariable(m_CompilePC + 4,_PROGRAM_COUNTER,"PROGRAM_COUNTER");
			UpdateCounters(&m_RegWorkingSet.BlockCycleCount(),&m_RegWorkingSet.BlockRandomModifier(),FALSE);
			m_RegWorkingSet.WriteBackRegisters();
			if (CPU_Type == CPU_SyncCores) { Call_Direct(SyncToPC, "SyncToPC"); }
			MoveConstToVariable(DELAY_SLOT,&m_NextInstruction,"m_NextInstruction");
			Ret();
			m_NextInstruction = END_BLOCK;
#endif
			return;
		}
		if (DelaySlotEffectsCompare(m_CompilePC,m_Opcode.rs,0)) {
			if (IsConst(m_Opcode.rs)) { 
				MoveConstToVariable(cMipsRegLo(m_Opcode.rs),_PROGRAM_COUNTER, "PROGRAM_COUNTER");
			} else 	if (IsMapped(m_Opcode.rs)) { 
				MoveX86regToVariable(cMipsRegMapLo(m_Opcode.rs),_PROGRAM_COUNTER, "PROGRAM_COUNTER");
			} else {
				MoveX86regToVariable(Map_TempReg(x86_Any,m_Opcode.rs,FALSE),_PROGRAM_COUNTER, "PROGRAM_COUNTER");
			}
		}
		m_NextInstruction = DO_DELAY_SLOT;
	} else if (m_NextInstruction == DELAY_SLOT_DONE ) {		
		if (DelaySlotEffectsCompare(m_CompilePC,m_Opcode.rs,0)) {
			m_Section->CompileExit(m_CompilePC,(DWORD)-1,m_RegWorkingSet,CExitInfo::Normal,TRUE,NULL);
		} else {
			if (IsConst(m_Opcode.rs)) { 
				memcpy(&m_Section->m_Jump.RegSet,&m_RegWorkingSet,sizeof(CRegInfo));
				m_Section->GenerateSectionLinkage();
			} else {
				if (IsMapped(m_Opcode.rs)) { 
					MoveX86regToVariable(cMipsRegMapLo(m_Opcode.rs),_PROGRAM_COUNTER, "PROGRAM_COUNTER");
				} else {
					MoveX86regToVariable(Map_TempReg(x86_Any,m_Opcode.rs,FALSE),_PROGRAM_COUNTER, "PROGRAM_COUNTER");
				}
				m_Section->CompileExit(-1, (DWORD)-1,m_RegWorkingSet,CExitInfo::Normal,TRUE,NULL);
			}
		}
		m_NextInstruction = END_BLOCK;
	} else {
#ifndef EXTERNAL_RELEASE
		DisplayError("WTF\n\nBranch\nNextInstruction = %X", m_NextInstruction);
#endif
	}
}

void CRecompilerOps::SPECIAL_JALR (void) {
	if ( m_NextInstruction == NORMAL ) {
		CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
		if (DelaySlotEffectsCompare(m_CompilePC,m_Opcode.rs,0)) {
			CRecompilerOps::UnknownOpcode();
		}
		UnMap_GPR( m_Opcode.rd, FALSE);
		MipsRegLo(m_Opcode.rd) = m_CompilePC + 8;
		MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		if ((m_CompilePC & 0xFFC) == 0xFFC) {
_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
			if (IsMapped(m_Opcode.rs)) { 
				MoveX86regToVariable(cMipsRegLo(m_Opcode.rs),&JumpToLocation, "JumpToLocation");
			} else {
				MoveX86regToVariable(Map_TempReg(x86_Any,m_Opcode.rs,FALSE),&JumpToLocation, "JumpToLocation");
			}
			MoveConstToVariable(m_CompilePC + 4,_PROGRAM_COUNTER,"PROGRAM_COUNTER");
			UpdateCounters(&m_RegWorkingSet.BlockCycleCount(),&m_RegWorkingSet.BlockRandomModifier(),FALSE);
			m_RegWorkingSet.WriteBackRegisters();
			if (CPU_Type == CPU_SyncCores) { Call_Direct(SyncToPC, "SyncToPC"); }
			MoveConstToVariable(DELAY_SLOT,&m_NextInstruction,"m_NextInstruction");
			Ret();
#endif
			m_NextInstruction = END_BLOCK;
			return;
		}
		m_NextInstruction = DO_DELAY_SLOT;
	} else if (m_NextInstruction == DELAY_SLOT_DONE ) {		
		if (IsConst(m_Opcode.rs)) { 
			memcpy(&m_Section->m_Jump.RegSet,&m_RegWorkingSet,sizeof(CRegInfo));
			m_Section->m_Jump.BranchLabel.Format("0x%08X",cMipsRegLo(m_Opcode.rs));
			m_Section->m_Jump.TargetPC      = cMipsRegLo(m_Opcode.rs);
			m_Section->m_Jump.FallThrough   = TRUE;
			m_Section->m_Jump.LinkLocation  = NULL;
			m_Section->m_Jump.LinkLocation2 = NULL;
			m_Section->m_Cont.FallThrough   = FALSE;
			m_Section->m_Cont.LinkLocation  = NULL;
			m_Section->m_Cont.LinkLocation2 = NULL;

			m_Section->GenerateSectionLinkage();
		} else {
			if (IsMapped(m_Opcode.rs)) { 
				MoveX86regToVariable(cMipsRegMapLo(m_Opcode.rs),_PROGRAM_COUNTER, "PROGRAM_COUNTER");
			} else {
				MoveX86regToVariable(Map_TempReg(x86_Any,m_Opcode.rs,FALSE),_PROGRAM_COUNTER, "PROGRAM_COUNTER");
			}
			m_Section->CompileExit(m_CompilePC, (DWORD)-1,m_RegWorkingSet,CExitInfo::Normal,TRUE,NULL);
		}
		m_NextInstruction = END_BLOCK;
	} else {
#ifndef EXTERNAL_RELEASE
		DisplayError("WTF\n\nBranch\nNextInstruction = %X", m_NextInstruction);
#endif
	}
}

void CRecompilerOps::SPECIAL_SYSCALL (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	m_Section->CompileExit(m_CompilePC,m_CompilePC,m_RegWorkingSet,CExitInfo::DoSysCall,TRUE,NULL);
#endif
}

void CRecompilerOps::SPECIAL_MFLO (void) {
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rd == 0) { return; }

	Map_GPR_64bit(m_Opcode.rd,-1);
	MoveVariableToX86reg(&_RegLO->UW[0],"_RegLO->UW[0]",cMipsRegMapLo(m_Opcode.rd));
	MoveVariableToX86reg(&_RegLO->UW[1],"_RegLO->UW[1]",cMipsRegMapHi(m_Opcode.rd));
}

void CRecompilerOps::SPECIAL_MTLO (void) {
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	if (m_Section->IsKnown(m_Opcode.rs) && IsConst(m_Opcode.rs)) {
		if (Is64Bit(m_Opcode.rs)) {
			MoveConstToVariable(MipsRegHi(m_Opcode.rs),&_RegLO->UW[1],"_RegLO->UW[1]");
		} else if (m_Section->IsSigned(m_Opcode.rs) && ((cMipsRegLo(m_Opcode.rs) & 0x80000000) != 0)) {
			MoveConstToVariable(0xFFFFFFFF,&_RegLO->UW[1],"_RegLO->UW[1]");
		} else {
			MoveConstToVariable(0,&_RegLO->UW[1],"_RegLO->UW[1]");
		}
		MoveConstToVariable(cMipsRegLo(m_Opcode.rs), &_RegLO->UW[0],"_RegLO->UW[0]");
	} else if (m_Section->IsKnown(m_Opcode.rs) && IsMapped(m_Opcode.rs)) {
		if (Is64Bit(m_Opcode.rs)) {
			MoveX86regToVariable(MipsRegMapHi(m_Opcode.rs),&_RegLO->UW[1],"_RegLO->UW[1]");
		} else if (m_Section->IsSigned(m_Opcode.rs)) {
			MoveX86regToVariable(Map_TempReg(x86_Any,m_Opcode.rs,TRUE),&_RegLO->UW[1],"_RegLO->UW[1]");
		} else {
			MoveConstToVariable(0,&_RegLO->UW[1],"_RegLO->UW[1]");
		}
		MoveX86regToVariable(cMipsRegMapLo(m_Opcode.rs), &_RegLO->UW[0],"_RegLO->UW[0]");
	} else {
		x86Reg reg = Map_TempReg(x86_Any,m_Opcode.rs,TRUE);
		MoveX86regToVariable(reg,&_RegLO->UW[1],"_RegLO->UW[1]");
		MoveX86regToVariable(Map_TempReg(reg,m_Opcode.rs,FALSE), &_RegLO->UW[0],"_RegLO->UW[0]");
	}
}

void CRecompilerOps::SPECIAL_MFHI (void) {
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rd == 0) { return; }

	Map_GPR_64bit(m_Opcode.rd,-1);
	MoveVariableToX86reg(&_RegHI->UW[0],"_RegHI->UW[0]",cMipsRegMapLo(m_Opcode.rd));
	MoveVariableToX86reg(&_RegHI->UW[1],"_RegHI->UW[1]",cMipsRegMapHi(m_Opcode.rd));
}

void CRecompilerOps::SPECIAL_MTHI (void) {
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Section->IsKnown(m_Opcode.rs) && IsConst(m_Opcode.rs)) {
		if (Is64Bit(m_Opcode.rs)) {
			MoveConstToVariable(MipsRegHi(m_Opcode.rs),&_RegHI->UW[1],"_RegHI->UW[1]");
		} else if (m_Section->IsSigned(m_Opcode.rs) && ((cMipsRegLo(m_Opcode.rs) & 0x80000000) != 0)) {
			MoveConstToVariable(0xFFFFFFFF,&_RegHI->UW[1],"_RegHI->UW[1]");
		} else {
			MoveConstToVariable(0,&_RegHI->UW[1],"_RegHI->UW[1]");
		}
		MoveConstToVariable(cMipsRegLo(m_Opcode.rs), &_RegHI->UW[0],"_RegHI->UW[0]");
	} else if (m_Section->IsKnown(m_Opcode.rs) && IsMapped(m_Opcode.rs)) {
		if (Is64Bit(m_Opcode.rs)) {
			MoveX86regToVariable(MipsRegMapHi(m_Opcode.rs),&_RegHI->UW[1],"_RegHI->UW[1]");
		} else if (m_Section->IsSigned(m_Opcode.rs)) {
			MoveX86regToVariable(Map_TempReg(x86_Any,m_Opcode.rs,TRUE),&_RegHI->UW[1],"_RegHI->UW[1]");
		} else {
			MoveConstToVariable(0,&_RegHI->UW[1],"_RegHI->UW[1]");
		}
		MoveX86regToVariable(cMipsRegMapLo(m_Opcode.rs), &_RegHI->UW[0],"_RegHI->UW[0]");
	} else {
		x86Reg reg = Map_TempReg(x86_Any,m_Opcode.rs,TRUE);
		MoveX86regToVariable(reg,&_RegHI->UW[1],"_RegHI->UW[1]");
		MoveX86regToVariable(Map_TempReg(reg,m_Opcode.rs,FALSE), &_RegHI->UW[0],"_RegHI->UW[0]");
	}
}

void CRecompilerOps::SPECIAL_DSLLV (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	BYTE * Jump[2];

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rd == 0) { return; }
	
	if (IsConst(m_Opcode.rs)) {
		DWORD Shift = (cMipsRegLo(m_Opcode.rs) & 0x3F);
		CRecompilerOps::UnknownOpcode();
		return;
	}
	Map_TempReg(x86_ECX,m_Opcode.rs,FALSE);
	AndConstToX86Reg(x86_ECX,0x3F);
	Map_GPR_64bit(m_Opcode.rd,m_Opcode.rt);
	CompConstToX86reg(x86_ECX,0x20);
	JaeLabel8("MORE32", 0);
	Jump[0] = m_RecompPos - 1;
	ShiftLeftDouble(MipsRegHi(m_Opcode.rd),cMipsRegLo(m_Opcode.rd));
	ShiftLeftSign(cMipsRegLo(m_Opcode.rd));
	JmpLabel8("continue", 0);
	Jump[1] = m_RecompPos - 1;
	
	//MORE32:
	CPU_Message("");
	CPU_Message("      MORE32:");
	*((BYTE *)(Jump[0]))=(BYTE)(m_RecompPos - Jump[0] - 1);
	MoveX86RegToX86Reg(cMipsRegLo(m_Opcode.rd),MipsRegHi(m_Opcode.rd));
	XorX86RegToX86Reg(cMipsRegLo(m_Opcode.rd),cMipsRegLo(m_Opcode.rd));
	AndConstToX86Reg(x86_ECX,0x1F);
	ShiftLeftSign(MipsRegHi(m_Opcode.rd));

	//continue:
	CPU_Message("");
	CPU_Message("      continue:");
	*((BYTE *)(Jump[1]))=(BYTE)(m_RecompPos - Jump[1] - 1);
#endif
}

void CRecompilerOps::SPECIAL_DSRLV (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	BYTE * Jump[2];

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rd == 0) { return; }
	
	if (IsConst(m_Opcode.rs)) {
		DWORD Shift = (cMipsRegLo(m_Opcode.rs) & 0x3F);
		if (IsConst(m_Opcode.rt)) {
			if (IsMapped(m_Opcode.rd)) { UnMap_GPR(m_Opcode.rd, FALSE); }
			m_Section->MipsReg(m_Opcode.rd) = Is64Bit(m_Opcode.rt)?m_Section->MipsReg(m_Opcode.rt):(__int64)m_Section->MipsRegLo_S(m_Opcode.rt);
			m_Section->MipsReg(m_Opcode.rd) = m_Section->MipsReg(m_Opcode.rd) >> Shift;
			if ((MipsRegHi(m_Opcode.rd) == 0) && (cMipsRegLo(m_Opcode.rd) & 0x80000000) == 0) {
				MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
			} else if ((MipsRegHi(m_Opcode.rd) == 0xFFFFFFFF) && (cMipsRegLo(m_Opcode.rd) & 0x80000000) != 0) {
				MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
			} else {
				MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_64;
			}
			return;
		}
		//if (Shift < 0x20) {
		//} else {
		//}
		//CRecompilerOps::UnknownOpcode();
		//return;
	}
	Map_TempReg(x86_ECX,m_Opcode.rs,FALSE);
	AndConstToX86Reg(x86_ECX,0x3F);
	Map_GPR_64bit(m_Opcode.rd,m_Opcode.rt);
	CompConstToX86reg(x86_ECX,0x20);
	JaeLabel8("MORE32", 0);
	Jump[0] = m_RecompPos - 1;
	ShiftRightDouble(cMipsRegLo(m_Opcode.rd),MipsRegHi(m_Opcode.rd));
	ShiftRightUnsign(MipsRegHi(m_Opcode.rd));
	JmpLabel8("continue", 0);
	Jump[1] = m_RecompPos - 1;
	
	//MORE32:
	CPU_Message("");
	CPU_Message("      MORE32:");
	*((BYTE *)(Jump[0]))=(BYTE)(m_RecompPos - Jump[0] - 1);
	MoveX86RegToX86Reg(MipsRegHi(m_Opcode.rd),cMipsRegLo(m_Opcode.rd));
	XorX86RegToX86Reg(MipsRegHi(m_Opcode.rd),MipsRegHi(m_Opcode.rd));
	AndConstToX86Reg(x86_ECX,0x1F);
	ShiftRightUnsign(cMipsRegLo(m_Opcode.rd));

	//continue:
	CPU_Message("");
	CPU_Message("      continue:");
	*((BYTE *)(Jump[1]))=(BYTE)(m_RecompPos - Jump[1] - 1);
#endif
}

void CRecompilerOps::SPECIAL_DSRAV (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	BYTE * Jump[2];

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rd == 0) { return; }
	
	if (IsConst(m_Opcode.rs)) {
		DWORD Shift = (cMipsRegLo(m_Opcode.rs) & 0x3F);
		CRecompilerOps::UnknownOpcode();
		return;
	}
	Map_TempReg(x86_ECX,m_Opcode.rs,FALSE);
	AndConstToX86Reg(x86_ECX,0x3F);
	Map_GPR_64bit(m_Opcode.rd,m_Opcode.rt);
	CompConstToX86reg(x86_ECX,0x20);
	JaeLabel8("MORE32", 0);
	Jump[0] = m_RecompPos - 1;
	ShiftRightDouble(cMipsRegLo(m_Opcode.rd),MipsRegHi(m_Opcode.rd));
	ShiftRightSign(MipsRegHi(m_Opcode.rd));
	JmpLabel8("continue", 0);
	Jump[1] = m_RecompPos - 1;
	
	//MORE32:
	CPU_Message("");
	CPU_Message("      MORE32:");
	*((BYTE *)(Jump[0]))=(BYTE)(m_RecompPos - Jump[0] - 1);
	MoveX86RegToX86Reg(MipsRegHi(m_Opcode.rd),cMipsRegLo(m_Opcode.rd));
	ShiftRightSignImmed(MipsRegHi(m_Opcode.rd),0x1F);
	AndConstToX86Reg(x86_ECX,0x1F);
	ShiftRightSign(cMipsRegLo(m_Opcode.rd));

	//continue:
	CPU_Message("");
	CPU_Message("      continue:");
	*((BYTE *)(Jump[1]))=(BYTE)(m_RecompPos - Jump[1] - 1);
#endif
}

void CRecompilerOps::SPECIAL_MULT ( void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	cX86Protected(x86_EDX) = TRUE;
	Map_TempReg(x86_EAX,m_Opcode.rs,FALSE);
	cX86Protected(x86_EDX) = FALSE;
	Map_TempReg(x86_EDX,m_Opcode.rt,FALSE);

	imulX86reg(x86_EDX);

	MoveX86regToVariable(x86_EAX,&_RegLO->UW[0],"_RegLO->UW[0]");
	MoveX86regToVariable(x86_EDX,&_RegHI->UW[0],"_RegHI->UW[0]");
	ShiftRightSignImmed(x86_EAX,31);	/* paired */
	ShiftRightSignImmed(x86_EDX,31);
	MoveX86regToVariable(x86_EAX,&_RegLO->UW[1],"_RegLO->UW[1]");
	MoveX86regToVariable(x86_EDX,&_RegHI->UW[1],"_RegHI->UW[1]");
#endif
}

void CRecompilerOps::SPECIAL_MULTU (void) {
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	X86Protected(x86_EDX) = TRUE;
	Map_TempReg(x86_EAX,m_Opcode.rs,FALSE);
	X86Protected(x86_EDX) = FALSE;
	Map_TempReg(x86_EDX,m_Opcode.rt,FALSE);

	MulX86reg(x86_EDX);

	MoveX86regToVariable(x86_EAX,&_RegLO->UW[0],"_RegLO->UW[0]");
	MoveX86regToVariable(x86_EDX,&_RegHI->UW[0],"_RegHI->UW[0]");
	ShiftRightSignImmed(x86_EAX,31);	/* paired */
	ShiftRightSignImmed(x86_EDX,31);
	MoveX86regToVariable(x86_EAX,&_RegLO->UW[1],"_RegLO->UW[1]");
	MoveX86regToVariable(x86_EDX,&_RegHI->UW[1],"_RegHI->UW[1]");
}

void CRecompilerOps::SPECIAL_DIV (void) {
	BYTE *Jump[2];

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	
	if (IsConst(m_Opcode.rt)) {
		if (MipsRegLo(m_Opcode.rt) == 0) {
			MoveConstToVariable(0, &_RegLO->UW[0], "_RegLO->UW[0]");
			MoveConstToVariable(0, &_RegLO->UW[1], "_RegLO->UW[1]");
			MoveConstToVariable(0, &_RegHI->UW[0], "_RegHI->UW[0]");
			MoveConstToVariable(0, &_RegHI->UW[1], "_RegHI->UW[1]");
			return;
		}
		Jump[1] = NULL;
	} else {
		if (IsMapped(m_Opcode.rt)) {
			CompConstToX86reg(cMipsRegMapLo(m_Opcode.rt),0);
		} else {
			CompConstToVariable(0, &_GPR[m_Opcode.rt].W[0], CRegName::GPR_Lo[m_Opcode.rt]);
		}
		JneLabel8("NoExcept", 0);
		Jump[0] = m_RecompPos - 1;

		MoveConstToVariable(0, &_RegLO->UW[0], "_RegLO->UW[0]");
		MoveConstToVariable(0, &_RegLO->UW[1], "_RegLO->UW[1]");
		MoveConstToVariable(0, &_RegHI->UW[0], "_RegHI->UW[0]");
		MoveConstToVariable(0, &_RegHI->UW[1], "_RegHI->UW[1]");

		JmpLabel8("EndDivu", 0);
		Jump[1] = m_RecompPos - 1;
		
		CPU_Message("");
		CPU_Message("      NoExcept:");
		*((BYTE *)(Jump[0]))=(BYTE)(m_RecompPos - Jump[0] - 1);
	}
	/*	lo = (SD)rs / (SD)rt;
		hi = (SD)rs % (SD)rt; */

	X86Protected(x86_EDX) = TRUE;
	Map_TempReg(x86_EAX,m_Opcode.rs,FALSE);

	/* edx is the signed portion to eax */
	X86Protected(x86_EDX) = FALSE;
	Map_TempReg(x86_EDX, -1, FALSE);

	MoveX86RegToX86Reg(x86_EAX, x86_EDX);
	ShiftRightSignImmed(x86_EDX,31);

	if (IsMapped(m_Opcode.rt)) {
		idivX86reg(cMipsRegMapLo(m_Opcode.rt));
	} else {
		idivX86reg(Map_TempReg(x86_Any,m_Opcode.rt,FALSE));
	}
		

	MoveX86regToVariable(x86_EAX,&_RegLO->UW[0],"_RegLO->UW[0]");
	MoveX86regToVariable(x86_EDX,&_RegHI->UW[0],"_RegHI->UW[0]");
	ShiftRightSignImmed(x86_EAX,31);	/* paired */
	ShiftRightSignImmed(x86_EDX,31);
	MoveX86regToVariable(x86_EAX,&_RegLO->UW[1],"_RegLO->UW[1]");
	MoveX86regToVariable(x86_EDX,&_RegHI->UW[1],"_RegHI->UW[1]");

	if( Jump[1] != NULL ) {
		CPU_Message("");
		CPU_Message("      EndDivu:");
		*((BYTE *)(Jump[1]))=(BYTE)(m_RecompPos - Jump[1] - 1);
	}
}

void CRecompilerOps::SPECIAL_DIVU ( void) {
	BYTE *Jump[2];
	x86Reg Reg;

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	if (IsConst(m_Opcode.rt)) {
		if (MipsRegLo(m_Opcode.rt) == 0) {
			MoveConstToVariable(0, &_RegLO->UW[0], "_RegLO->UW[0]");
			MoveConstToVariable(0, &_RegLO->UW[1], "_RegLO->UW[1]");
			MoveConstToVariable(0, &_RegHI->UW[0], "_RegHI->UW[0]");
			MoveConstToVariable(0, &_RegHI->UW[1], "_RegHI->UW[1]");
			return;
		}
		Jump[1] = NULL;
	} else {
		if (IsMapped(m_Opcode.rt)) {
			CompConstToX86reg(cMipsRegMapLo(m_Opcode.rt),0);
		} else {
			CompConstToVariable(0, &_GPR[m_Opcode.rt].W[0], CRegName::GPR_Lo[m_Opcode.rt]);
		}
		JneLabel8("NoExcept", 0);
		Jump[0] = m_RecompPos - 1;

		MoveConstToVariable(0, &_RegLO->UW[0], "_RegLO->UW[0]");
		MoveConstToVariable(0, &_RegLO->UW[1], "_RegLO->UW[1]");
		MoveConstToVariable(0, &_RegHI->UW[0], "_RegHI->UW[0]");
		MoveConstToVariable(0, &_RegHI->UW[1], "_RegHI->UW[1]");

		JmpLabel8("EndDivu", 0);
		Jump[1] = m_RecompPos - 1;
		
		CPU_Message("");
		CPU_Message("      NoExcept:");
		*((BYTE *)(Jump[0]))=(BYTE)(m_RecompPos - Jump[0] - 1);
	}


	/*	lo = (UD)rs / (UD)rt;
		hi = (UD)rs % (UD)rt; */

	X86Protected(x86_EAX) = TRUE;
	Map_TempReg(x86_EDX, 0, FALSE);
	X86Protected(x86_EAX) = FALSE;

	Map_TempReg(x86_EAX,m_Opcode.rs,FALSE);
	Reg = Map_TempReg(x86_Any,m_Opcode.rt,FALSE);

	DivX86reg(Reg);

	MoveX86regToVariable(x86_EAX,&_RegLO->UW[0],"_RegLO->UW[0]");
	MoveX86regToVariable(x86_EDX,&_RegHI->UW[0],"_RegHI->UW[0]");

	/* wouldnt these be zero (???) */

	ShiftRightSignImmed(x86_EAX,31);	/* paired */
	ShiftRightSignImmed(x86_EDX,31);
	MoveX86regToVariable(x86_EAX,&_RegLO->UW[1],"_RegLO->UW[1]");
	MoveX86regToVariable(x86_EDX,&_RegHI->UW[1],"_RegHI->UW[1]");

	if( Jump[1] != NULL ) {
		CPU_Message("");
		CPU_Message("      EndDivu:");
		*((BYTE *)(Jump[1]))=(BYTE)(m_RecompPos - Jump[1] - 1);
	}
}

void CRecompilerOps::SPECIAL_DMULT (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	if (m_Opcode.rs != 0) { UnMap_GPR(m_Opcode.rs,TRUE); }
	if (m_Opcode.rs != 0) { UnMap_GPR(m_Opcode.rt,TRUE); }
	Pushad();
	MoveConstToVariable(m_Opcode.Hex, &R4300iOp::m_Opcode.Hex, "R4300iOp::m_Opcode.Hex");
	Call_Direct(R4300iOp::SPECIAL_DMULT, "R4300iOp::SPECIAL_DMULT");
	Popad();
#endif
}

void CRecompilerOps::SPECIAL_DMULTU (void) {
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	/* _RegLO->UDW = (uint64)_GPR[m_Opcode.rs].UW[0] * (uint64)_GPR[m_Opcode.rt].UW[0]; */
	X86Protected(x86_EDX) = TRUE;
	Map_TempReg(x86_EAX,m_Opcode.rs,FALSE);
	X86Protected(x86_EDX) = FALSE;
	Map_TempReg(x86_EDX,m_Opcode.rt,FALSE);

	MulX86reg(x86_EDX);
	MoveX86regToVariable(x86_EAX, &_RegLO->UW[0], "_RegLO->UW[0]");
	MoveX86regToVariable(x86_EDX, &_RegLO->UW[1], "_RegLO->UW[1]");

	/* _RegHI->UDW = (uint64)_GPR[m_Opcode.rs].UW[1] * (uint64)_GPR[m_Opcode.rt].UW[1]; */
	Map_TempReg(x86_EAX,m_Opcode.rs,TRUE);
	Map_TempReg(x86_EDX,m_Opcode.rt,TRUE);

	MulX86reg(x86_EDX);
	MoveX86regToVariable(x86_EAX, &_RegHI->UW[0], "_RegHI->UW[0]");
	MoveX86regToVariable(x86_EDX, &_RegHI->UW[1], "_RegHI->UW[1]");

	/* Tmp[0].UDW = (uint64)_GPR[m_Opcode.rs].UW[1] * (uint64)_GPR[m_Opcode.rt].UW[0]; */
	Map_TempReg(x86_EAX,m_Opcode.rs,TRUE);
	Map_TempReg(x86_EDX,m_Opcode.rt,FALSE);

	Map_TempReg(x86_EBX,-1,FALSE);
	Map_TempReg(x86_ECX,-1,FALSE);

	MulX86reg(x86_EDX);
	MoveX86RegToX86Reg(x86_EAX, x86_EBX); /* EDX:EAX -> ECX:EBX */
	MoveX86RegToX86Reg(x86_EDX, x86_ECX);

	/* Tmp[1].UDW = (uint64)_GPR[m_Opcode.rs].UW[0] * (uint64)_GPR[m_Opcode.rt].UW[1]; */
	Map_TempReg(x86_EAX,m_Opcode.rs,FALSE);
	Map_TempReg(x86_EDX,m_Opcode.rt,TRUE);

	MulX86reg(x86_EDX);
	Map_TempReg(x86_ESI,-1,FALSE);
	Map_TempReg(x86_EDI,-1,FALSE);
	MoveX86RegToX86Reg(x86_EAX, x86_ESI); /* EDX:EAX -> EDI:ESI */
	MoveX86RegToX86Reg(x86_EDX, x86_EDI);

	/* Tmp[2].UDW = (uint64)_RegLO->UW[1] + (uint64)Tmp[0].UW[0] + (uint64)Tmp[1].UW[0]; */
	XorX86RegToX86Reg(x86_EDX, x86_EDX);
	MoveVariableToX86reg(&_RegLO->UW[1], "_RegLO->UW[1]", x86_EAX);
	AddX86RegToX86Reg(x86_EAX, x86_EBX);
	AddConstToX86Reg(x86_EDX, 0);
	AddX86RegToX86Reg(x86_EAX, x86_ESI);
	AddConstToX86Reg(x86_EDX, 0);			/* EDX:EAX */

	/* _RegLO->UDW += ((uint64)Tmp[0].UW[0] + (uint64)Tmp[1].UW[0]) << 32; */
	/* [low+4] += ebx + esi */

	AddX86regToVariable(x86_EBX, &_RegLO->UW[1], "_RegLO->UW[1]");
	AddX86regToVariable(x86_ESI, &_RegLO->UW[1], "_RegLO->UW[1]");

	/* _RegHI->UDW += (uint64)Tmp[0].UW[1] + (uint64)Tmp[1].UW[1] + Tmp[2].UW[1]; */
	/* [hi] += ecx + edi + edx */
	
	AddX86regToVariable(x86_ECX, &_RegHI->UW[0], "_RegHI->UW[0]");
	AdcConstToVariable(&_RegHI->UW[1], "_RegHI->UW[1]", 0);

	AddX86regToVariable(x86_EDI, &_RegHI->UW[0], "_RegHI->UW[0]");
	AdcConstToVariable(&_RegHI->UW[1], "_RegHI->UW[1]", 0);

	AddX86regToVariable(x86_EDX, &_RegHI->UW[0], "_RegHI->UW[0]");
	AdcConstToVariable(&_RegHI->UW[1], "_RegHI->UW[1]", 0);
}

void CRecompilerOps::SPECIAL_DDIV (void) {
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	UnMap_GPR(m_Opcode.rs,TRUE);
	UnMap_GPR(m_Opcode.rt,TRUE);
	Pushad();
	MoveConstToVariable(m_Opcode.Hex, &R4300iOp::m_Opcode.Hex, "R4300iOp::m_Opcode.Hex");
	Call_Direct(R4300iOp::SPECIAL_DDIV, "R4300iOp::SPECIAL_DDIV");
	Popad();
}

void CRecompilerOps::SPECIAL_DDIVU (void) 
{
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	UnMap_GPR(m_Opcode.rs,TRUE);
	UnMap_GPR(m_Opcode.rt,TRUE);
	Pushad();
	MoveConstToVariable(m_Opcode.Hex, &R4300iOp::m_Opcode.Hex, "R4300iOp::m_Opcode.Hex");
	Call_Direct(R4300iOp::SPECIAL_DDIVU, "R4300iOp::SPECIAL_DDIVU");
	Popad();
}

void CRecompilerOps::SPECIAL_ADD (void) {
	int source1 = m_Opcode.rd == m_Opcode.rt?m_Opcode.rt:m_Opcode.rs;
	int source2 = m_Opcode.rd == m_Opcode.rt?m_Opcode.rs:m_Opcode.rt;

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rd == 0) { return; }

	if (IsConst(source1) && IsConst(source2)) {
		DWORD temp = cMipsRegLo(source1) + cMipsRegLo(source2);
		if (IsMapped(m_Opcode.rd)) { UnMap_GPR(m_Opcode.rd, FALSE); }
		MipsRegLo(m_Opcode.rd) = temp;
		MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		return;
	}

	Map_GPR_32bit(m_Opcode.rd,TRUE, source1);
	if (IsConst(source2)) {
		AddConstToX86Reg(cMipsRegMapLo(m_Opcode.rd),cMipsRegLo(source2));
	} else if (m_Section->IsKnown(source2) && IsMapped(source2)) {
		AddX86RegToX86Reg(cMipsRegMapLo(m_Opcode.rd),cMipsRegMapLo(source2));
	} else {
		AddVariableToX86reg(cMipsRegMapLo(m_Opcode.rd),&_GPR[source2].W[0],CRegName::GPR_Lo[source2]);
	}
}

void CRecompilerOps::SPECIAL_ADDU (void) {
	int source1 = m_Opcode.rd == m_Opcode.rt?m_Opcode.rt:m_Opcode.rs;
	int source2 = m_Opcode.rd == m_Opcode.rt?m_Opcode.rs:m_Opcode.rt;

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rd == 0) { return; }

	if (IsConst(source1) && IsConst(source2)) {
		DWORD temp = cMipsRegLo(source1) + cMipsRegLo(source2);
		if (IsMapped(m_Opcode.rd)) { UnMap_GPR(m_Opcode.rd, FALSE); }
		MipsRegLo(m_Opcode.rd) = temp;
		MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		return;
	}

	Map_GPR_32bit(m_Opcode.rd,TRUE, source1);
	if (IsConst(source2)) {
		AddConstToX86Reg(cMipsRegMapLo(m_Opcode.rd),cMipsRegLo(source2));
	} else if (m_Section->IsKnown(source2) && IsMapped(source2)) {
		AddX86RegToX86Reg(cMipsRegMapLo(m_Opcode.rd),cMipsRegMapLo(source2));
	} else {
		AddVariableToX86reg(cMipsRegMapLo(m_Opcode.rd),&_GPR[source2].W[0],CRegName::GPR_Lo[source2]);
	}
}

void CRecompilerOps::SPECIAL_SUB (void) {
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rd == 0) { return; }

	if (IsConst(m_Opcode.rt)  && IsConst(m_Opcode.rs)) {
		DWORD temp = cMipsRegLo(m_Opcode.rs) - cMipsRegLo(m_Opcode.rt);
		if (IsMapped(m_Opcode.rd)) { UnMap_GPR(m_Opcode.rd, FALSE); }
		MipsRegLo(m_Opcode.rd) = temp;
		MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
	} else {
		if (m_Opcode.rd == m_Opcode.rt) {
			x86Reg Reg = Map_TempReg(x86_Any,m_Opcode.rt,FALSE);
			Map_GPR_32bit(m_Opcode.rd,TRUE, m_Opcode.rs);
			SubX86RegToX86Reg(cMipsRegMapLo(m_Opcode.rd),Reg);
			return;
		}
		Map_GPR_32bit(m_Opcode.rd,TRUE, m_Opcode.rs);
		if (IsConst(m_Opcode.rt)) {
			SubConstFromX86Reg(cMipsRegMapLo(m_Opcode.rd),cMipsRegLo(m_Opcode.rt));
		} else if (IsMapped(m_Opcode.rt)) {
			SubX86RegToX86Reg(cMipsRegMapLo(m_Opcode.rd),cMipsRegMapLo(m_Opcode.rt));
		} else {
			SubVariableFromX86reg(cMipsRegMapLo(m_Opcode.rd),&_GPR[m_Opcode.rt].W[0],CRegName::GPR_Lo[m_Opcode.rt]);
		}
	}
}

void CRecompilerOps::SPECIAL_SUBU (void) {
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rd == 0) { return; }

	if (IsConst(m_Opcode.rt)  && IsConst(m_Opcode.rs)) {
		DWORD temp = cMipsRegLo(m_Opcode.rs) - cMipsRegLo(m_Opcode.rt);
		if (IsMapped(m_Opcode.rd)) { UnMap_GPR(m_Opcode.rd, FALSE); }
		MipsRegLo(m_Opcode.rd) = temp;
		MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
	} else {
		if (m_Opcode.rd == m_Opcode.rt) {
			x86Reg Reg = Map_TempReg(x86_Any,m_Opcode.rt,FALSE);
			Map_GPR_32bit(m_Opcode.rd,TRUE, m_Opcode.rs);
			SubX86RegToX86Reg(cMipsRegMapLo(m_Opcode.rd),Reg);
			return;
		}
		Map_GPR_32bit(m_Opcode.rd,TRUE, m_Opcode.rs);
		if (IsConst(m_Opcode.rt)) {
			SubConstFromX86Reg(cMipsRegMapLo(m_Opcode.rd),cMipsRegLo(m_Opcode.rt));
		} else if (IsMapped(m_Opcode.rt)) {
			SubX86RegToX86Reg(cMipsRegMapLo(m_Opcode.rd),cMipsRegMapLo(m_Opcode.rt));
		} else {
			SubVariableFromX86reg(cMipsRegMapLo(m_Opcode.rd),&_GPR[m_Opcode.rt].W[0],CRegName::GPR_Lo[m_Opcode.rt]);
		}
	}
}

void CRecompilerOps::SPECIAL_AND (void) 
{
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Section->IsKnown(m_Opcode.rt) && m_Section->IsKnown(m_Opcode.rs)) {
		if (IsConst(m_Opcode.rt) && IsConst(m_Opcode.rs)) {
			if (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs)) {
				m_Section->MipsReg(m_Opcode.rd) = 
					(Is64Bit(m_Opcode.rt)?m_Section->MipsReg(m_Opcode.rt):(__int64)m_Section->MipsRegLo_S(m_Opcode.rt)) &
					(Is64Bit(m_Opcode.rs)?m_Section->MipsReg(m_Opcode.rs):(__int64)m_Section->MipsRegLo_S(m_Opcode.rs));
				
				if (m_Section->MipsRegLo_S(m_Opcode.rd) < 0 && MipsRegHi_S(m_Opcode.rd) == -1){ 
					MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
				} else if (m_Section->MipsRegLo_S(m_Opcode.rd) >= 0 && MipsRegHi_S(m_Opcode.rd) == 0){ 
					MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
				} else {
					MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_64;
				}
			} else {
				m_Section->MipsReg(m_Opcode.rd) = cMipsRegLo(m_Opcode.rt) & m_Section->MipsReg(m_Opcode.rs);
				MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
			}			
		} else if (IsMapped(m_Opcode.rt) && IsMapped(m_Opcode.rs)) {
			int source1 = m_Opcode.rd == m_Opcode.rt?m_Opcode.rt:m_Opcode.rs;
			int source2 = m_Opcode.rd == m_Opcode.rt?m_Opcode.rs:m_Opcode.rt;
		
			ProtectGPR(source1);
			ProtectGPR(source2);
			if (m_Section->Is32Bit(source1) && m_Section->Is32Bit(source2)) {
				int Sign = (m_Section->IsSigned(m_Opcode.rt) && m_Section->IsSigned(m_Opcode.rs))?TRUE:FALSE;
				Map_GPR_32bit(m_Opcode.rd,Sign,source1);				
				AndX86RegToX86Reg(cMipsRegMapLo(m_Opcode.rd),cMipsRegMapLo(source2));
			} else if (m_Section->Is32Bit(source1) || m_Section->Is32Bit(source2)) {
				if (m_Section->IsUnsigned(m_Section->Is32Bit(source1)?source1:source2)) {
					Map_GPR_32bit(m_Opcode.rd,FALSE,source1);
					AndX86RegToX86Reg(cMipsRegMapLo(m_Opcode.rd),cMipsRegMapLo(source2));
				} else {
					Map_GPR_64bit(m_Opcode.rd,source1);
					if (m_Section->Is32Bit(source2)) {
						AndX86RegToX86Reg(MipsRegMapHi(m_Opcode.rd),Map_TempReg(x86_Any,source2,TRUE));
					} else {
						AndX86RegToX86Reg(MipsRegMapHi(m_Opcode.rd),MipsRegMapHi(source2));
					}
					AndX86RegToX86Reg(cMipsRegMapLo(m_Opcode.rd),cMipsRegMapLo(source2));
				}
			} else {
				Map_GPR_64bit(m_Opcode.rd,source1);
				AndX86RegToX86Reg(cMipsRegMapHi(m_Opcode.rd),cMipsRegMapHi(source2));
				AndX86RegToX86Reg(cMipsRegMapLo(m_Opcode.rd),cMipsRegMapLo(source2));
			}
		} else {
			int ConstReg = IsConst(m_Opcode.rt)?m_Opcode.rt:m_Opcode.rs;
			int MappedReg = IsConst(m_Opcode.rt)?m_Opcode.rs:m_Opcode.rt;

			if (Is64Bit(ConstReg)) {
				if (m_Section->Is32Bit(MappedReg) && m_Section->IsUnsigned(MappedReg)) {
					if (cMipsRegLo(ConstReg) == 0) {
						Map_GPR_32bit(m_Opcode.rd,FALSE, 0);
					} else {
						DWORD Value = cMipsRegLo(ConstReg);
						Map_GPR_32bit(m_Opcode.rd,FALSE, MappedReg);
						AndConstToX86Reg(cMipsRegMapLo(m_Opcode.rd),Value);
					}
				} else {
					__int64 Value = m_Section->MipsReg(ConstReg);
					Map_GPR_64bit(m_Opcode.rd,MappedReg);
					AndConstToX86Reg(cMipsRegMapHi(m_Opcode.rd),(DWORD)(Value >> 32));
					AndConstToX86Reg(cMipsRegMapLo(m_Opcode.rd),(DWORD)Value);
				}
			} else if (Is64Bit(MappedReg)) {
				DWORD Value = cMipsRegLo(ConstReg); 
				if (Value != 0) {
					Map_GPR_32bit(m_Opcode.rd,m_Section->IsSigned(ConstReg)?TRUE:FALSE,MappedReg);					
					AndConstToX86Reg(cMipsRegMapLo(m_Opcode.rd),(DWORD)Value);
				} else {
					Map_GPR_32bit(m_Opcode.rd,m_Section->IsSigned(ConstReg)?TRUE:FALSE, 0);
				}
			} else {
				DWORD Value = cMipsRegLo(ConstReg); 
				int Sign = FALSE;
				if (m_Section->IsSigned(ConstReg) && m_Section->IsSigned(MappedReg)) { Sign = TRUE; }				
				if (Value != 0) {
					Map_GPR_32bit(m_Opcode.rd,Sign,MappedReg);
					AndConstToX86Reg(cMipsRegMapLo(m_Opcode.rd),Value);
				} else {
					Map_GPR_32bit(m_Opcode.rd,FALSE, 0);
				}
			}
		}
	} else if (m_Section->IsKnown(m_Opcode.rt) || m_Section->IsKnown(m_Opcode.rs)) {
		DWORD KnownReg = m_Section->IsKnown(m_Opcode.rt)?m_Opcode.rt:m_Opcode.rs;
		DWORD UnknownReg = m_Section->IsKnown(m_Opcode.rt)?m_Opcode.rs:m_Opcode.rt;

		if (IsConst(KnownReg)) {
			if (Is64Bit(KnownReg)) {
				unsigned __int64 Value = m_Section->MipsReg(KnownReg);
				Map_GPR_64bit(m_Opcode.rd,UnknownReg);
				AndConstToX86Reg(cMipsRegMapHi(m_Opcode.rd),(DWORD)(Value >> 32));
				AndConstToX86Reg(cMipsRegMapLo(m_Opcode.rd),(DWORD)Value);
			} else {
				DWORD Value = cMipsRegLo(KnownReg);
				Map_GPR_32bit(m_Opcode.rd,m_Section->IsSigned(KnownReg),UnknownReg);
				AndConstToX86Reg(cMipsRegMapLo(m_Opcode.rd),(DWORD)Value);
			}
		} else {
			ProtectGPR(KnownReg);
			if (KnownReg == m_Opcode.rd) {
				if (Is64Bit(KnownReg)) {
					Map_GPR_64bit(m_Opcode.rd,KnownReg);
					AndVariableToX86Reg(&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg],MipsRegMapHi(m_Opcode.rd));
					AndVariableToX86Reg(&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg],cMipsRegMapLo(m_Opcode.rd));
				} else {
					Map_GPR_32bit(m_Opcode.rd,m_Section->IsSigned(KnownReg),KnownReg);
					AndVariableToX86Reg(&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg],cMipsRegMapLo(m_Opcode.rd));
				}
			} else {
				if (Is64Bit(KnownReg)) {
					Map_GPR_64bit(m_Opcode.rd,UnknownReg);
					AndX86RegToX86Reg(cMipsRegMapHi(m_Opcode.rd),cMipsRegMapHi(KnownReg));
					AndX86RegToX86Reg(cMipsRegMapLo(m_Opcode.rd),cMipsRegMapLo(KnownReg));
				} else {
					Map_GPR_32bit(m_Opcode.rd,m_Section->IsSigned(KnownReg),UnknownReg);
					AndX86RegToX86Reg(cMipsRegMapLo(m_Opcode.rd),cMipsRegMapLo(KnownReg));
				}
			}
		}
	} else {
		Map_GPR_64bit(m_Opcode.rd,m_Opcode.rt);
		AndVariableToX86Reg(&_GPR[m_Opcode.rs].W[1],CRegName::GPR_Hi[m_Opcode.rs],cMipsRegMapHi(m_Opcode.rd));
		AndVariableToX86Reg(&_GPR[m_Opcode.rs].W[0],CRegName::GPR_Lo[m_Opcode.rs],cMipsRegMapLo(m_Opcode.rd));
	}
}

void CRecompilerOps::SPECIAL_OR (void) {
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	if (m_Section->IsKnown(m_Opcode.rt) && m_Section->IsKnown(m_Opcode.rs)) {
		if (IsConst(m_Opcode.rt) && IsConst(m_Opcode.rs)) {
			if (IsMapped(m_Opcode.rd)) { UnMap_GPR(m_Opcode.rd, FALSE); }
			if (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs)) {				
				m_Section->MipsReg(m_Opcode.rd) = 
					(Is64Bit(m_Opcode.rt)?m_Section->MipsReg(m_Opcode.rt):(__int64)m_Section->MipsRegLo_S(m_Opcode.rt)) |
					(Is64Bit(m_Opcode.rs)?m_Section->MipsReg(m_Opcode.rs):(__int64)m_Section->MipsRegLo_S(m_Opcode.rs));
				if (m_Section->MipsRegLo_S(m_Opcode.rd) < 0 && MipsRegHi_S(m_Opcode.rd) == -1){ 
					MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
				} else if (m_Section->MipsRegLo_S(m_Opcode.rd) >= 0 && MipsRegHi_S(m_Opcode.rd) == 0){ 
					MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
				} else {
					MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_64;
				}
			} else {
				MipsRegLo(m_Opcode.rd) = cMipsRegLo(m_Opcode.rt) | cMipsRegLo(m_Opcode.rs);
				MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
			}
		} else if (IsMapped(m_Opcode.rt) && IsMapped(m_Opcode.rs)) {
			int source1 = m_Opcode.rd == m_Opcode.rt?m_Opcode.rt:m_Opcode.rs;
			int source2 = m_Opcode.rd == m_Opcode.rt?m_Opcode.rs:m_Opcode.rt;
			
			ProtectGPR(m_Opcode.rt);
			ProtectGPR(m_Opcode.rs);
			if (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs)) {
				Map_GPR_64bit(m_Opcode.rd,source1);
				if (Is64Bit(source2)) {
					OrX86RegToX86Reg(MipsRegMapHi(m_Opcode.rd),MipsRegMapHi(source2));
				} else {
					OrX86RegToX86Reg(MipsRegMapHi(m_Opcode.rd),Map_TempReg(x86_Any,source2,TRUE));
				}
			} else {
				ProtectGPR(source2);
				Map_GPR_32bit(m_Opcode.rd,TRUE,source1);
			}
			OrX86RegToX86Reg(cMipsRegMapLo(m_Opcode.rd),cMipsRegMapLo(source2));
		} else {
			DWORD ConstReg = IsConst(m_Opcode.rt)?m_Opcode.rt:m_Opcode.rs;
			DWORD MappedReg = IsConst(m_Opcode.rt)?m_Opcode.rs:m_Opcode.rt;

			if (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs)) {
				unsigned __int64 Value;

				if (Is64Bit(ConstReg)) {
					Value = m_Section->MipsReg(ConstReg);
				} else {
					Value = m_Section->IsSigned(ConstReg)?m_Section->MipsRegLo_S(ConstReg):cMipsRegLo(ConstReg);
				}
				Map_GPR_64bit(m_Opcode.rd,MappedReg);
				if ((Value >> 32) != 0) {
					OrConstToX86Reg((DWORD)(Value >> 32),MipsRegMapHi(m_Opcode.rd));
				}
				if ((DWORD)Value != 0) {
					OrConstToX86Reg((DWORD)Value,cMipsRegMapLo(m_Opcode.rd));
				}
			} else {
				int Value = cMipsRegLo(ConstReg);
				Map_GPR_32bit(m_Opcode.rd,TRUE, MappedReg);
				if (Value != 0) { OrConstToX86Reg(Value,cMipsRegMapLo(m_Opcode.rd)); }
			}
		}
	} else if (m_Section->IsKnown(m_Opcode.rt) || m_Section->IsKnown(m_Opcode.rs)) {
		int KnownReg = m_Section->IsKnown(m_Opcode.rt)?m_Opcode.rt:m_Opcode.rs;
		int UnknownReg = m_Section->IsKnown(m_Opcode.rt)?m_Opcode.rs:m_Opcode.rt;
		
		if (IsConst(KnownReg)) {
			unsigned __int64 Value;

			Value = Is64Bit(KnownReg)?m_Section->MipsReg(KnownReg):m_Section->MipsRegLo_S(KnownReg);
			Map_GPR_64bit(m_Opcode.rd,UnknownReg);
			if ((Value >> 32) != 0) {
				OrConstToX86Reg((DWORD)(Value >> 32),MipsRegMapHi(m_Opcode.rd));
			}
			if ((DWORD)Value != 0) {
				OrConstToX86Reg((DWORD)Value,cMipsRegMapLo(m_Opcode.rd));
			}
		} else {
			Map_GPR_64bit(m_Opcode.rd,KnownReg);
			OrVariableToX86Reg(&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg],MipsRegMapHi(m_Opcode.rd));
			OrVariableToX86Reg(&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg],cMipsRegMapLo(m_Opcode.rd));
		}
	} else {
		Map_GPR_64bit(m_Opcode.rd,m_Opcode.rt);
		OrVariableToX86Reg(&_GPR[m_Opcode.rs].W[1],CRegName::GPR_Hi[m_Opcode.rs],MipsRegMapHi(m_Opcode.rd));
		OrVariableToX86Reg(&_GPR[m_Opcode.rs].W[0],CRegName::GPR_Lo[m_Opcode.rs],cMipsRegMapLo(m_Opcode.rd));
	}
#ifdef tofix
	if (SPHack && m_Opcode.rd == 29) { 
		ResetX86Protection();
		_MMU->ResetMemoryStack(m_Section); 
	}
#endif
}

void CRecompilerOps::SPECIAL_XOR (void) {
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rd == 0) { return; }

	if (m_Opcode.rt == m_Opcode.rs) {
		UnMap_GPR( m_Opcode.rd, FALSE);
		MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		MipsRegLo(m_Opcode.rd) = 0;
		return;
	}
	if (m_Section->IsKnown(m_Opcode.rt) && m_Section->IsKnown(m_Opcode.rs)) {
		if (IsConst(m_Opcode.rt) && IsConst(m_Opcode.rs)) {
			if (IsMapped(m_Opcode.rd)) { UnMap_GPR(m_Opcode.rd, FALSE); }
			if (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs)) {
#ifndef EXTERNAL_RELEASE
				DisplayError("XOR 1");
#endif
				CRecompilerOps::UnknownOpcode();
			} else {
				MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
				MipsRegLo(m_Opcode.rd) = cMipsRegLo(m_Opcode.rt) ^ cMipsRegLo(m_Opcode.rs);
			}
		} else if (IsMapped(m_Opcode.rt) && IsMapped(m_Opcode.rs)) {
			int source1 = m_Opcode.rd == m_Opcode.rt?m_Opcode.rt:m_Opcode.rs;
			int source2 = m_Opcode.rd == m_Opcode.rt?m_Opcode.rs:m_Opcode.rt;
			
			ProtectGPR(source1);
			ProtectGPR(source2);
			if (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs)) {
				Map_GPR_64bit(m_Opcode.rd,source1);
				if (Is64Bit(source2)) {
					XorX86RegToX86Reg(cMipsRegMapHi(m_Opcode.rd),cMipsRegMapHi(source2));
				} else if (m_Section->IsSigned(source2)) {
					XorX86RegToX86Reg(cMipsRegMapHi(m_Opcode.rd),Map_TempReg(x86_Any,source2,TRUE));
				}
				XorX86RegToX86Reg(cMipsRegMapLo(m_Opcode.rd),cMipsRegMapLo(source2));
			} else {
				if (m_Section->IsSigned(m_Opcode.rt) != m_Section->IsSigned(m_Opcode.rs)) {
					Map_GPR_32bit(m_Opcode.rd,TRUE,source1);
				} else {
					Map_GPR_32bit(m_Opcode.rd,m_Section->IsSigned(m_Opcode.rt),source1);
				}
				XorX86RegToX86Reg(cMipsRegMapLo(m_Opcode.rd),cMipsRegMapLo(source2));
			}
		} else {
			DWORD ConstReg = IsConst(m_Opcode.rt)?m_Opcode.rt:m_Opcode.rs;
			DWORD MappedReg = IsConst(m_Opcode.rt)?m_Opcode.rs:m_Opcode.rt;

			if (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs)) {
				DWORD ConstHi, ConstLo;

				ConstHi = m_Section->Is32Bit(ConstReg)?(DWORD)(m_Section->MipsRegLo_S(ConstReg) >> 31):MipsRegHi(ConstReg);
				ConstLo = cMipsRegLo(ConstReg);
				Map_GPR_64bit(m_Opcode.rd,MappedReg);
				if (ConstHi != 0) { XorConstToX86Reg(cMipsRegMapHi(m_Opcode.rd),ConstHi); }
				if (ConstLo != 0) { XorConstToX86Reg(cMipsRegMapLo(m_Opcode.rd),ConstLo); }
			} else {
				int Value = cMipsRegLo(ConstReg);
				if (m_Section->IsSigned(m_Opcode.rt) != m_Section->IsSigned(m_Opcode.rs)) {
					Map_GPR_32bit(m_Opcode.rd,TRUE, MappedReg);
				} else {
					Map_GPR_32bit(m_Opcode.rd,m_Section->IsSigned(MappedReg)?TRUE:FALSE, MappedReg);
				}
				if (Value != 0) { XorConstToX86Reg(cMipsRegMapLo(m_Opcode.rd),Value); }
			}
		}
	} else if (m_Section->IsKnown(m_Opcode.rt) || m_Section->IsKnown(m_Opcode.rs)) {
		int KnownReg = m_Section->IsKnown(m_Opcode.rt)?m_Opcode.rt:m_Opcode.rs;
		int UnknownReg = m_Section->IsKnown(m_Opcode.rt)?m_Opcode.rs:m_Opcode.rt;
		
		if (IsConst(KnownReg)) {
			unsigned __int64 Value;

			if (Is64Bit(KnownReg)) {
				Value = m_Section->MipsReg(KnownReg);
			} else {
				if (m_Section->IsSigned(KnownReg)) {
					Value = (int)cMipsRegLo(KnownReg);
				} else {
					Value = cMipsRegLo(KnownReg);
				}
			}
			Map_GPR_64bit(m_Opcode.rd,UnknownReg);
			if ((Value >> 32) != 0) {
				XorConstToX86Reg(MipsRegMapHi(m_Opcode.rd),(DWORD)(Value >> 32));
			}
			if ((DWORD)Value != 0) {
				XorConstToX86Reg(cMipsRegMapLo(m_Opcode.rd),(DWORD)Value);
			}
		} else {
			Map_GPR_64bit(m_Opcode.rd,KnownReg);
			XorVariableToX86reg(&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg],cMipsRegMapHi(m_Opcode.rd));
			XorVariableToX86reg(&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg],cMipsRegMapLo(m_Opcode.rd));
		}
	} else {
		Map_GPR_64bit(m_Opcode.rd,m_Opcode.rt);
		XorVariableToX86reg(&_GPR[m_Opcode.rs].W[1],CRegName::GPR_Hi[m_Opcode.rs],cMipsRegMapHi(m_Opcode.rd));
		XorVariableToX86reg(&_GPR[m_Opcode.rs].W[0],CRegName::GPR_Lo[m_Opcode.rs],cMipsRegMapLo(m_Opcode.rd));
	}
}

void CRecompilerOps::SPECIAL_NOR (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	if (m_Section->IsKnown(m_Opcode.rt) && m_Section->IsKnown(m_Opcode.rs)) {
		if (IsConst(m_Opcode.rt) && IsConst(m_Opcode.rs)) {
			if (IsMapped(m_Opcode.rd)) { UnMap_GPR(m_Opcode.rd, FALSE); }
			if (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs)) {				
				CRecompilerOps::UnknownOpcode();
			} else {
				MipsRegLo(m_Opcode.rd) = ~(cMipsRegLo(m_Opcode.rt) | cMipsRegLo(m_Opcode.rs));
				MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
			}
		} else if (IsMapped(m_Opcode.rt) && IsMapped(m_Opcode.rs)) {
			int source1 = m_Opcode.rd == m_Opcode.rt?m_Opcode.rt:m_Opcode.rs;
			int source2 = m_Opcode.rd == m_Opcode.rt?m_Opcode.rs:m_Opcode.rt;
			
			ProtectGPR(source1);
			ProtectGPR(source2);
			if (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs)) {
				Map_GPR_64bit(m_Opcode.rd,source1);
				if (Is64Bit(source2)) {
					OrX86RegToX86Reg(cMipsRegMapHi(m_Opcode.rd),cMipsRegMapHi(source2));
				} else {
					OrX86RegToX86Reg(cMipsRegMapHi(m_Opcode.rd),Map_TempReg(x86_Any,source2,TRUE));
				}
				OrX86RegToX86Reg(cMipsRegMapLo(m_Opcode.rd),cMipsRegMapLo(source2));
				NotX86Reg(cMipsRegMapHi(m_Opcode.rd));
				NotX86Reg(cMipsRegMapLo(m_Opcode.rd));
			} else {
				ProtectGPR(source2);
				if (m_Section->IsSigned(m_Opcode.rt) != m_Section->IsSigned(m_Opcode.rs)) {
					Map_GPR_32bit(m_Opcode.rd,TRUE,source1);
				} else {
					Map_GPR_32bit(m_Opcode.rd,m_Section->IsSigned(m_Opcode.rt),source1);
				}
				OrX86RegToX86Reg(cMipsRegMapLo(m_Opcode.rd),cMipsRegMapLo(source2));
				NotX86Reg(cMipsRegLo(m_Opcode.rd));
			}
		} else {
			DWORD ConstReg = IsConst(m_Opcode.rt)?m_Opcode.rt:m_Opcode.rs;
			DWORD MappedReg = IsConst(m_Opcode.rt)?m_Opcode.rs:m_Opcode.rt;

			if (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs)) {
				unsigned __int64 Value;

				if (Is64Bit(ConstReg)) {
					Value = m_Section->MipsReg(ConstReg);
				} else {
					Value = m_Section->IsSigned(ConstReg)?m_Section->MipsRegLo_S(ConstReg):cMipsRegLo(ConstReg);
				}
				Map_GPR_64bit(m_Opcode.rd,MappedReg);
				if ((Value >> 32) != 0) {
					OrConstToX86Reg((DWORD)(Value >> 32),MipsRegHi(m_Opcode.rd));
				}
				if ((DWORD)Value != 0) {
					OrConstToX86Reg((DWORD)Value,cMipsRegLo(m_Opcode.rd));
				}
				NotX86Reg(MipsRegHi(m_Opcode.rd));
				NotX86Reg(cMipsRegLo(m_Opcode.rd));
			} else {
				int Value = cMipsRegLo(ConstReg);
				if (m_Section->IsSigned(m_Opcode.rt) != m_Section->IsSigned(m_Opcode.rs)) {
					Map_GPR_32bit(m_Opcode.rd,TRUE, MappedReg);
				} else {
					Map_GPR_32bit(m_Opcode.rd,m_Section->IsSigned(MappedReg)?TRUE:FALSE, MappedReg);
				}
				if (Value != 0) { OrConstToX86Reg(Value,cMipsRegLo(m_Opcode.rd)); }
				NotX86Reg(cMipsRegLo(m_Opcode.rd));
			}
		}
	} else if (m_Section->IsKnown(m_Opcode.rt) || m_Section->IsKnown(m_Opcode.rs)) {
		int KnownReg = m_Section->IsKnown(m_Opcode.rt)?m_Opcode.rt:m_Opcode.rs;
		int UnknownReg = m_Section->IsKnown(m_Opcode.rt)?m_Opcode.rs:m_Opcode.rt;
		
		if (IsConst(KnownReg)) {
			unsigned __int64 Value;

			Value = Is64Bit(KnownReg)?m_Section->MipsReg(KnownReg):m_Section->MipsRegLo_S(KnownReg);
			Map_GPR_64bit(m_Opcode.rd,UnknownReg);
			if ((Value >> 32) != 0) {
				OrConstToX86Reg((DWORD)(Value >> 32),MipsRegHi(m_Opcode.rd));
			}
			if ((DWORD)Value != 0) {
				OrConstToX86Reg((DWORD)Value,cMipsRegLo(m_Opcode.rd));
			}
		} else {
			Map_GPR_64bit(m_Opcode.rd,KnownReg);
			OrVariableToX86Reg(&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg],MipsRegHi(m_Opcode.rd));
			OrVariableToX86Reg(&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg],cMipsRegLo(m_Opcode.rd));
		}
		NotX86Reg(MipsRegHi(m_Opcode.rd));
		NotX86Reg(cMipsRegLo(m_Opcode.rd));
	} else {
		Map_GPR_64bit(m_Opcode.rd,m_Opcode.rt);
		OrVariableToX86Reg(&_GPR[m_Opcode.rs].W[1],CRegName::GPR_Hi[m_Opcode.rs],MipsRegHi(m_Opcode.rd));
		OrVariableToX86Reg(&_GPR[m_Opcode.rs].W[0],CRegName::GPR_Lo[m_Opcode.rs],cMipsRegLo(m_Opcode.rd));
		NotX86Reg(MipsRegHi(m_Opcode.rd));
		NotX86Reg(cMipsRegLo(m_Opcode.rd));
	}
#endif
}

void CRecompilerOps::SPECIAL_SLT (void) {
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	if (m_Section->IsKnown(m_Opcode.rt) && m_Section->IsKnown(m_Opcode.rs)) {
		if (IsConst(m_Opcode.rt) && IsConst(m_Opcode.rs)) {
			if (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs)) {
				DisplayError("1");
				CRecompilerOps::UnknownOpcode();
			} else {
				if (IsMapped(m_Opcode.rd)) { UnMap_GPR(m_Opcode.rd, FALSE); }
				MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;	
				if (m_Section->MipsRegLo_S(m_Opcode.rs) < m_Section->MipsRegLo_S(m_Opcode.rt)) {
					MipsRegLo(m_Opcode.rd) = 1;
				} else {
					MipsRegLo(m_Opcode.rd) = 0;
				}
			}
		} else if (IsMapped(m_Opcode.rt) && IsMapped(m_Opcode.rs)) {
			ProtectGPR(m_Opcode.rt);
			ProtectGPR(m_Opcode.rs);
			if (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs)) {
				BYTE *Jump[2];

				CompX86RegToX86Reg(
					Is64Bit(m_Opcode.rs)?cMipsRegMapHi(m_Opcode.rs):Map_TempReg(x86_Any,m_Opcode.rs,TRUE), 
					Is64Bit(m_Opcode.rt)?cMipsRegMapHi(m_Opcode.rt):Map_TempReg(x86_Any,m_Opcode.rt,TRUE)
				);
				JeLabel8("Low Compare",0);
				Jump[0] = m_RecompPos - 1;
				SetlVariable(&m_BranchCompare,"m_BranchCompare");
				JmpLabel8("Continue",0);
				Jump[1] = m_RecompPos - 1;
				
				CPU_Message("");
				CPU_Message("      Low Compare:");
				*((BYTE *)(Jump[0]))=(BYTE)(m_RecompPos - Jump[0] - 1);
				CompX86RegToX86Reg(cMipsRegMapLo(m_Opcode.rs), cMipsRegMapLo(m_Opcode.rt));
				SetbVariable(&m_BranchCompare,"m_BranchCompare");
				CPU_Message("");
				CPU_Message("      Continue:");
				*((BYTE *)(Jump[1]))=(BYTE)(m_RecompPos - Jump[1] - 1);
				Map_GPR_32bit(m_Opcode.rd,TRUE, -1);
				MoveVariableToX86reg(&m_BranchCompare,"m_BranchCompare",cMipsRegMapLo(m_Opcode.rd));
			} else {
				Map_GPR_32bit(m_Opcode.rd,TRUE, -1);
				CompX86RegToX86Reg(cMipsRegMapLo(m_Opcode.rs), cMipsRegMapLo(m_Opcode.rt));

				if (cMipsRegMapLo(m_Opcode.rd) > x86_EBX) {
					SetlVariable(&m_BranchCompare,"m_BranchCompare");
					MoveVariableToX86reg(&m_BranchCompare,"m_BranchCompare",cMipsRegMapLo(m_Opcode.rd));
				} else {					
					Setl(cMipsRegMapLo(m_Opcode.rd));
					AndConstToX86Reg(cMipsRegMapLo(m_Opcode.rd), 1);
				}
			}
		} else {
			DWORD ConstReg  = IsConst(m_Opcode.rs)?m_Opcode.rs:m_Opcode.rt;
			DWORD MappedReg = IsConst(m_Opcode.rs)?m_Opcode.rt:m_Opcode.rs;

			ProtectGPR(MappedReg);
			if (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs)) {
				BYTE *Jump[2];

				CompConstToX86reg(
					Is64Bit(MappedReg)?cMipsRegMapHi(MappedReg):Map_TempReg(x86_Any,MappedReg,TRUE), 
					Is64Bit(ConstReg)?MipsRegHi(ConstReg):(m_Section->MipsRegLo_S(ConstReg) >> 31)
				);
				JeLabel8("Low Compare",0);
				Jump[0] = m_RecompPos - 1;
				if (MappedReg == m_Opcode.rs) {
					SetlVariable(&m_BranchCompare,"m_BranchCompare");
				} else {
					SetgVariable(&m_BranchCompare,"m_BranchCompare");
				}
				JmpLabel8("Continue",0);
				Jump[1] = m_RecompPos - 1;
				
				CPU_Message("");
				CPU_Message("      Low Compare:");
				*((BYTE *)(Jump[0]))=(BYTE)(m_RecompPos - Jump[0] - 1);
				CompConstToX86reg(cMipsRegMapLo(MappedReg), cMipsRegLo(ConstReg));
				if (MappedReg == m_Opcode.rs) {
					SetbVariable(&m_BranchCompare,"m_BranchCompare");
				} else {
					SetaVariable(&m_BranchCompare,"m_BranchCompare");
				}
				CPU_Message("");
				CPU_Message("      Continue:");
				*((BYTE *)(Jump[1]))=(BYTE)(m_RecompPos - Jump[1] - 1);
				Map_GPR_32bit(m_Opcode.rd,TRUE, -1);
				MoveVariableToX86reg(&m_BranchCompare,"m_BranchCompare",cMipsRegMapLo(m_Opcode.rd));
			} else {
				DWORD Constant = cMipsRegLo(ConstReg);
				Map_GPR_32bit(m_Opcode.rd,TRUE, -1);
				CompConstToX86reg(cMipsRegMapLo(MappedReg), Constant);
			
				if (cMipsRegMapLo(m_Opcode.rd) > x86_EBX) {
					if (MappedReg == m_Opcode.rs) {
						SetlVariable(&m_BranchCompare,"m_BranchCompare");
					} else {
						SetgVariable(&m_BranchCompare,"m_BranchCompare");
					}
					MoveVariableToX86reg(&m_BranchCompare,"m_BranchCompare",cMipsRegMapLo(m_Opcode.rd));
				} else {					
					if (MappedReg == m_Opcode.rs) {
						Setl(cMipsRegMapLo(m_Opcode.rd));
					} else {
						Setg(cMipsRegMapLo(m_Opcode.rd));
					}
					AndConstToX86Reg(cMipsRegMapLo(m_Opcode.rd), 1);
				}
			}
		}		
	} else if (m_Section->IsKnown(m_Opcode.rt) || m_Section->IsKnown(m_Opcode.rs)) {
		DWORD KnownReg = m_Section->IsKnown(m_Opcode.rt)?m_Opcode.rt:m_Opcode.rs;
		DWORD UnknownReg = m_Section->IsKnown(m_Opcode.rt)?m_Opcode.rs:m_Opcode.rt;
		BYTE *Jump[2];
			
		if (IsConst(KnownReg)) {
			if (Is64Bit(KnownReg)) {
				CompConstToVariable(MipsRegHi(KnownReg),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else {
				CompConstToVariable((cMipsRegLo_S(KnownReg) >> 31),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			}
		} else {
			if (Is64Bit(KnownReg)) {
				CompX86regToVariable(cMipsRegMapHi(KnownReg),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else {
				ProtectGPR(KnownReg);
				CompX86regToVariable(Map_TempReg(x86_Any,KnownReg,TRUE),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			}			
		}
		JeLabel8("Low Compare",0);
		Jump[0] = m_RecompPos - 1;
		if (KnownReg == (IsConst(KnownReg)?m_Opcode.rs:m_Opcode.rt)) {
			SetgVariable(&m_BranchCompare,"m_BranchCompare");
		} else {
			SetlVariable(&m_BranchCompare,"m_BranchCompare");
		}
		JmpLabel8("Continue",0);
		Jump[1] = m_RecompPos - 1;
	
		CPU_Message("");
		CPU_Message("      Low Compare:");
		*((BYTE *)(Jump[0]))=(BYTE)(m_RecompPos - Jump[0] - 1);
		if (IsConst(KnownReg)) {
			CompConstToVariable(cMipsRegLo(KnownReg),&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg]);
		} else {
			CompX86regToVariable(cMipsRegMapLo(KnownReg),&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg]);
		}
		if (KnownReg == (IsConst(KnownReg)?m_Opcode.rs:m_Opcode.rt)) {
			SetaVariable(&m_BranchCompare,"m_BranchCompare");
		} else {
			SetbVariable(&m_BranchCompare,"m_BranchCompare");
		}
		CPU_Message("");
		CPU_Message("      Continue:");
		*((BYTE *)(Jump[1]))=(BYTE)(m_RecompPos - Jump[1] - 1);
		Map_GPR_32bit(m_Opcode.rd,TRUE, -1);
		MoveVariableToX86reg(&m_BranchCompare,"m_BranchCompare",cMipsRegMapLo(m_Opcode.rd));
	} else {
		BYTE *Jump[2];

		x86Reg Reg = Map_TempReg(x86_Any,m_Opcode.rs,TRUE);
		CompX86regToVariable(Reg,&_GPR[m_Opcode.rt].W[1],CRegName::GPR_Hi[m_Opcode.rt]);
		JeLabel8("Low Compare",0);
		Jump[0] = m_RecompPos - 1;
		SetlVariable(&m_BranchCompare,"m_BranchCompare");
		JmpLabel8("Continue",0);
		Jump[1] = m_RecompPos - 1;
		
		CPU_Message("");
		CPU_Message("      Low Compare:");
		*((BYTE *)(Jump[0]))=(BYTE)(m_RecompPos - Jump[0] - 1);
		CompX86regToVariable(Map_TempReg(Reg,m_Opcode.rs,FALSE),&_GPR[m_Opcode.rt].W[0],CRegName::GPR_Lo[m_Opcode.rt]);
		SetbVariable(&m_BranchCompare,"m_BranchCompare");
		CPU_Message("");
		CPU_Message("      Continue:");
		*((BYTE *)(Jump[1]))=(BYTE)(m_RecompPos - Jump[1] - 1);
		Map_GPR_32bit(m_Opcode.rd,TRUE, -1);
		MoveVariableToX86reg(&m_BranchCompare,"m_BranchCompare",cMipsRegMapLo(m_Opcode.rd));
	}
}

void CRecompilerOps::SPECIAL_SLTU (void) {
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rd == 0) { return; }

	if (m_Section->IsKnown(m_Opcode.rt) && m_Section->IsKnown(m_Opcode.rs)) {
		if (IsConst(m_Opcode.rt) && IsConst(m_Opcode.rs)) {
			if (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs)) {
#ifndef EXTERNAL_RELEASE
				DisplayError("1");
#endif
				CRecompilerOps::UnknownOpcode();
			} else {
				if (IsMapped(m_Opcode.rd)) { UnMap_GPR(m_Opcode.rd, FALSE); }
				MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;	
				if (cMipsRegLo(m_Opcode.rs) < cMipsRegLo(m_Opcode.rt)) {
					MipsRegLo(m_Opcode.rd) = 1;
				} else {
					MipsRegLo(m_Opcode.rd) = 0;
				}
			}
		} else if (IsMapped(m_Opcode.rt) && IsMapped(m_Opcode.rs)) {
			ProtectGPR(m_Opcode.rt);
			ProtectGPR(m_Opcode.rs);
			if (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs)) {
				BYTE *Jump[2];

				CompX86RegToX86Reg(
					Is64Bit(m_Opcode.rs)?cMipsRegMapHi(m_Opcode.rs):Map_TempReg(x86_Any,m_Opcode.rs,TRUE), 
					Is64Bit(m_Opcode.rt)?cMipsRegMapHi(m_Opcode.rt):Map_TempReg(x86_Any,m_Opcode.rt,TRUE)
				);
				JeLabel8("Low Compare",0);
				Jump[0] = m_RecompPos - 1;
				SetbVariable(&m_BranchCompare,"m_BranchCompare");
				JmpLabel8("Continue",0);
				Jump[1] = m_RecompPos - 1;
				
				CPU_Message("");
				CPU_Message("      Low Compare:");
				*((BYTE *)(Jump[0]))=(BYTE)(m_RecompPos - Jump[0] - 1);
				CompX86RegToX86Reg(cMipsRegMapLo(m_Opcode.rs), cMipsRegMapLo(m_Opcode.rt));
				SetbVariable(&m_BranchCompare,"m_BranchCompare");
				CPU_Message("");
				CPU_Message("      Continue:");
				*((BYTE *)(Jump[1]))=(BYTE)(m_RecompPos - Jump[1] - 1);
				Map_GPR_32bit(m_Opcode.rd,TRUE, -1);
				MoveVariableToX86reg(&m_BranchCompare,"m_BranchCompare",cMipsRegMapLo(m_Opcode.rd));
			} else {
				CompX86RegToX86Reg(cMipsRegMapLo(m_Opcode.rs), cMipsRegMapLo(m_Opcode.rt));
				SetbVariable(&m_BranchCompare,"m_BranchCompare");
				Map_GPR_32bit(m_Opcode.rd,TRUE, -1);
				MoveVariableToX86reg(&m_BranchCompare,"m_BranchCompare",cMipsRegMapLo(m_Opcode.rd));
			}
		} else {
			if (Is64Bit(m_Opcode.rt) || Is64Bit(m_Opcode.rs)) {
				DWORD ConstHi, ConstLo, ConstReg, MappedReg;
				x86Reg MappedRegHi, MappedRegLo;
				BYTE *Jump[2];

				ConstReg  = IsConst(m_Opcode.rt)?m_Opcode.rt:m_Opcode.rs;
				MappedReg = IsConst(m_Opcode.rt)?m_Opcode.rs:m_Opcode.rt;
				
				ConstLo = cMipsRegLo_S(ConstReg);
				ConstHi = cMipsRegLo_S(ConstReg) >> 31;
				if (Is64Bit(ConstReg)) { ConstHi = MipsRegHi(ConstReg); }

				ProtectGPR(MappedReg);
				MappedRegLo = cMipsRegMapLo(MappedReg);
				MappedRegHi = cMipsRegMapHi(MappedReg);
				if (m_Section->Is32Bit(MappedReg)) {
					MappedRegHi = Map_TempReg(x86_Any,MappedReg,TRUE);
				}

		
				Map_GPR_32bit(m_Opcode.rd,TRUE, -1);
				CompConstToX86reg(MappedRegHi, ConstHi);
				JeLabel8("Low Compare",0);
				Jump[0] = m_RecompPos - 1;
				if (MappedReg == m_Opcode.rs) {
					SetbVariable(&m_BranchCompare,"m_BranchCompare");
				} else {
					SetaVariable(&m_BranchCompare,"m_BranchCompare");
				}
				JmpLabel8("Continue",0);
				Jump[1] = m_RecompPos - 1;
	
				CPU_Message("");
				CPU_Message("      Low Compare:");
				*((BYTE *)(Jump[0]))=(BYTE)(m_RecompPos - Jump[0] - 1);
				CompConstToX86reg(MappedRegLo, ConstLo);
				if (MappedReg == m_Opcode.rs) {
					SetbVariable(&m_BranchCompare,"m_BranchCompare");
				} else {
					SetaVariable(&m_BranchCompare,"m_BranchCompare");
				}
				CPU_Message("");
				CPU_Message("      Continue:");
				*((BYTE *)(Jump[1]))=(BYTE)(m_RecompPos - Jump[1] - 1);
				Map_GPR_32bit(m_Opcode.rd,TRUE, -1);
				MoveVariableToX86reg(&m_BranchCompare,"m_BranchCompare",cMipsRegMapLo(m_Opcode.rd));
			} else {
				DWORD Const = IsConst(m_Opcode.rs)?cMipsRegLo(m_Opcode.rs):cMipsRegLo(m_Opcode.rt);
				DWORD MappedReg = IsConst(m_Opcode.rt)?m_Opcode.rs:m_Opcode.rt;

				CompConstToX86reg(cMipsRegMapLo(MappedReg), Const);
				if (MappedReg == m_Opcode.rs) {
					SetbVariable(&m_BranchCompare,"m_BranchCompare");
				} else {
					SetaVariable(&m_BranchCompare,"m_BranchCompare");
				}
				Map_GPR_32bit(m_Opcode.rd,TRUE, -1);
				MoveVariableToX86reg(&m_BranchCompare,"m_BranchCompare",cMipsRegMapLo(m_Opcode.rd));
			}
		}		
	} else if (m_Section->IsKnown(m_Opcode.rt) || m_Section->IsKnown(m_Opcode.rs)) {
		DWORD KnownReg = m_Section->IsKnown(m_Opcode.rt)?m_Opcode.rt:m_Opcode.rs;
		DWORD UnknownReg = m_Section->IsKnown(m_Opcode.rt)?m_Opcode.rs:m_Opcode.rt;
		BYTE *Jump[2];
			
		if (IsConst(KnownReg)) {
			if (Is64Bit(KnownReg)) {
				CompConstToVariable(MipsRegHi(KnownReg),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else {
				CompConstToVariable((cMipsRegLo_S(KnownReg) >> 31),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			}
		} else {
			if (Is64Bit(KnownReg)) {
				CompX86regToVariable(cMipsRegMapHi(KnownReg),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			} else {
				ProtectGPR(KnownReg);
				CompX86regToVariable(Map_TempReg(x86_Any,KnownReg,TRUE),&_GPR[UnknownReg].W[1],CRegName::GPR_Hi[UnknownReg]);
			}			
		}
		JeLabel8("Low Compare",0);
		Jump[0] = m_RecompPos - 1;
		if (KnownReg == (IsConst(KnownReg)?m_Opcode.rs:m_Opcode.rt)) {
			SetaVariable(&m_BranchCompare,"m_BranchCompare");
		} else {
			SetbVariable(&m_BranchCompare,"m_BranchCompare");
		}
		JmpLabel8("Continue",0);
		Jump[1] = m_RecompPos - 1;
	
		CPU_Message("");
		CPU_Message("      Low Compare:");
		*((BYTE *)(Jump[0]))=(BYTE)(m_RecompPos - Jump[0] - 1);
		if (IsConst(KnownReg)) {
			CompConstToVariable(cMipsRegLo(KnownReg),&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg]);
		} else {
			CompX86regToVariable(cMipsRegMapLo(KnownReg),&_GPR[UnknownReg].W[0],CRegName::GPR_Lo[UnknownReg]);
		}
		if (KnownReg == (IsConst(KnownReg)?m_Opcode.rs:m_Opcode.rt)) {
			SetaVariable(&m_BranchCompare,"m_BranchCompare");
		} else {
			SetbVariable(&m_BranchCompare,"m_BranchCompare");
		}
		CPU_Message("");
		CPU_Message("      Continue:");
		*((BYTE *)(Jump[1]))=(BYTE)(m_RecompPos - Jump[1] - 1);
		Map_GPR_32bit(m_Opcode.rd,TRUE, -1);
		MoveVariableToX86reg(&m_BranchCompare,"m_BranchCompare",cMipsRegMapLo(m_Opcode.rd));
	} else {
		BYTE *Jump[2];

		x86Reg Reg = Map_TempReg(x86_Any,m_Opcode.rs,TRUE);
		CompX86regToVariable(Reg,&_GPR[m_Opcode.rt].W[1],CRegName::GPR_Hi[m_Opcode.rt]);
		JeLabel8("Low Compare",0);
		Jump[0] = m_RecompPos - 1;
		SetbVariable(&m_BranchCompare,"m_BranchCompare");
		JmpLabel8("Continue",0);
		Jump[1] = m_RecompPos - 1;
		
		CPU_Message("");
		CPU_Message("      Low Compare:");
		*((BYTE *)(Jump[0]))=(BYTE)(m_RecompPos - Jump[0] - 1);
		CompX86regToVariable(Map_TempReg(Reg,m_Opcode.rs,FALSE),&_GPR[m_Opcode.rt].W[0],CRegName::GPR_Lo[m_Opcode.rt]);
		SetbVariable(&m_BranchCompare,"m_BranchCompare");
		CPU_Message("");
		CPU_Message("      Continue:");
		*((BYTE *)(Jump[1]))=(BYTE)(m_RecompPos - Jump[1] - 1);
		Map_GPR_32bit(m_Opcode.rd,TRUE, -1);
		MoveVariableToX86reg(&m_BranchCompare,"m_BranchCompare",cMipsRegMapLo(m_Opcode.rd));
	}
}

void CRecompilerOps::SPECIAL_DADD (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rd == 0) { return; }

	if (IsConst(m_Opcode.rt)  && IsConst(m_Opcode.rs)) {
		if (IsMapped(m_Opcode.rd)) { UnMap_GPR(m_Opcode.rd, FALSE); }
		m_Section->MipsReg(m_Opcode.rd) = 
			Is64Bit(m_Opcode.rs)?m_Section->MipsReg(m_Opcode.rs):(__int64)m_Section->MipsRegLo_S(m_Opcode.rs) +
			Is64Bit(m_Opcode.rt)?m_Section->MipsReg(m_Opcode.rt):(__int64)m_Section->MipsRegLo_S(m_Opcode.rt);		
		if (m_Section->MipsRegLo_S(m_Opcode.rd) < 0 && MipsRegHi_S(m_Opcode.rd) == -1){ 
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else if (m_Section->MipsRegLo_S(m_Opcode.rd) >= 0 && MipsRegHi_S(m_Opcode.rd) == 0){ 
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else {
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_64;
		}
	} else {
		int source1 = m_Opcode.rd == m_Opcode.rt?m_Opcode.rt:m_Opcode.rs;
		int source2 = m_Opcode.rd == m_Opcode.rt?m_Opcode.rs:m_Opcode.rt;

		Map_GPR_64bit(m_Opcode.rd,source1);
		if (IsConst(source2)) {
			AddConstToX86Reg(cMipsRegLo(m_Opcode.rd),cMipsRegLo(source2));
			AddConstToX86Reg(MipsRegHi(m_Opcode.rd),MipsRegHi(source2));
		} else if (IsMapped(source2)) {
			x86Reg HiReg = Is64Bit(source2)?MipsRegMapHi(source2):Map_TempReg(x86_Any,source2,TRUE);
			ProtectGPR(source2);
			AddX86RegToX86Reg(cMipsRegMapLo(m_Opcode.rd),cMipsRegMapLo(source2));
			AdcX86RegToX86Reg(cMipsRegMapHi(m_Opcode.rd),HiReg);
		} else {
			AddVariableToX86reg(cMipsRegLo(m_Opcode.rd),&_GPR[source2].W[0],CRegName::GPR_Lo[source2]);
			AdcVariableToX86reg(MipsRegHi(m_Opcode.rd),&_GPR[source2].W[1],CRegName::GPR_Hi[source2]);
		}
	}
#endif
}

void CRecompilerOps::SPECIAL_DADDU (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rd == 0) { return; }

	if (IsConst(m_Opcode.rt)  && IsConst(m_Opcode.rs)) {
		__int64 ValRs = Is64Bit(m_Opcode.rs)?m_Section->MipsReg(m_Opcode.rs):(__int64)m_Section->MipsRegLo_S(m_Opcode.rs);
		__int64 ValRt = Is64Bit(m_Opcode.rt)?m_Section->MipsReg(m_Opcode.rt):(__int64)m_Section->MipsRegLo_S(m_Opcode.rt);
		if (IsMapped(m_Opcode.rd)) { UnMap_GPR(m_Opcode.rd, FALSE); }
		m_Section->MipsReg(m_Opcode.rd) = ValRs + ValRt;
		if ((MipsRegHi(m_Opcode.rd) == 0) && (cMipsRegLo(m_Opcode.rd) & 0x80000000) == 0) {
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else if ((MipsRegHi(m_Opcode.rd) == 0xFFFFFFFF) && (cMipsRegLo(m_Opcode.rd) & 0x80000000) != 0) {
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else {
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_64;
		}
	} else {
		int source1 = m_Opcode.rd == m_Opcode.rt?m_Opcode.rt:m_Opcode.rs;
		int source2 = m_Opcode.rd == m_Opcode.rt?m_Opcode.rs:m_Opcode.rt;

		Map_GPR_64bit(m_Opcode.rd,source1);
		if (IsConst(source2)) {
			AddConstToX86Reg(cMipsRegLo(m_Opcode.rd),cMipsRegLo(source2));
			AddConstToX86Reg(MipsRegHi(m_Opcode.rd),MipsRegHi(source2));
		} else if (IsMapped(source2)) {
			x86Reg HiReg = Is64Bit(source2)?MipsRegMapHi(source2):Map_TempReg(x86_Any,source2,TRUE);
			ProtectGPR(source2);
			AddX86RegToX86Reg(cMipsRegMapLo(m_Opcode.rd),cMipsRegMapLo(source2));
			AdcX86RegToX86Reg(cMipsRegMapHi(m_Opcode.rd),HiReg);
		} else {
			AddVariableToX86reg(cMipsRegLo(m_Opcode.rd),&_GPR[source2].W[0],CRegName::GPR_Lo[source2]);
			AdcVariableToX86reg(MipsRegHi(m_Opcode.rd),&_GPR[source2].W[1],CRegName::GPR_Hi[source2]);
		}
	}
#endif
}

void CRecompilerOps::SPECIAL_DSUB (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rd == 0) { return; }

	if (IsConst(m_Opcode.rt)  && IsConst(m_Opcode.rs)) {
		if (IsMapped(m_Opcode.rd)) { UnMap_GPR(m_Opcode.rd, FALSE); }
		m_Section->MipsReg(m_Opcode.rd) = 
			Is64Bit(m_Opcode.rs)?m_Section->MipsReg(m_Opcode.rs):(__int64)m_Section->MipsRegLo_S(m_Opcode.rs) -
			Is64Bit(m_Opcode.rt)?m_Section->MipsReg(m_Opcode.rt):(__int64)m_Section->MipsRegLo_S(m_Opcode.rt);		
		if (m_Section->MipsRegLo_S(m_Opcode.rd) < 0 && MipsRegHi_S(m_Opcode.rd) == -1){ 
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else if (m_Section->MipsRegLo_S(m_Opcode.rd) >= 0 && MipsRegHi_S(m_Opcode.rd) == 0){ 
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else {
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_64;
		}
	} else {
		if (m_Opcode.rd == m_Opcode.rt) {
			x86Reg HiReg = Map_TempReg(x86_Any,m_Opcode.rt,TRUE);
			x86Reg LoReg = Map_TempReg(x86_Any,m_Opcode.rt,FALSE);
			Map_GPR_64bit(m_Opcode.rd,m_Opcode.rs);
			SubX86RegToX86Reg(cMipsRegLo(m_Opcode.rd),LoReg);
			SbbX86RegToX86Reg(cMipsRegMapHi(m_Opcode.rd),HiReg);
			return;
		}
		Map_GPR_64bit(m_Opcode.rd,m_Opcode.rs);
		if (IsConst(m_Opcode.rt)) {
			SubConstFromX86Reg(cMipsRegLo(m_Opcode.rd),cMipsRegLo(m_Opcode.rt));
			SbbConstFromX86Reg(MipsRegHi(m_Opcode.rd),MipsRegHi(m_Opcode.rt));
		} else if (IsMapped(m_Opcode.rt)) {
			x86Reg HiReg = Is64Bit(m_Opcode.rt)?cMipsRegMapHi(m_Opcode.rt):Map_TempReg(x86_Any,m_Opcode.rt,TRUE);
			ProtectGPR(m_Opcode.rt);
			SubX86RegToX86Reg(cMipsRegLo(m_Opcode.rd),cMipsRegLo(m_Opcode.rt));
			SbbX86RegToX86Reg(cMipsRegMapHi(m_Opcode.rd),HiReg);
		} else {
			SubVariableFromX86reg(cMipsRegLo(m_Opcode.rd),&_GPR[m_Opcode.rt].W[0],CRegName::GPR_Lo[m_Opcode.rt]);
			SbbVariableFromX86reg(MipsRegHi(m_Opcode.rd),&_GPR[m_Opcode.rt].W[1],CRegName::GPR_Hi[m_Opcode.rt]);
		}
	}
#endif
}

void CRecompilerOps::SPECIAL_DSUBU (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (m_Opcode.rd == 0) { return; }

	if (IsConst(m_Opcode.rt)  && IsConst(m_Opcode.rs)) {
		if (IsMapped(m_Opcode.rd)) { UnMap_GPR(m_Opcode.rd, FALSE); }
		m_Section->MipsReg(m_Opcode.rd) = 
			Is64Bit(m_Opcode.rs)?m_Section->MipsReg(m_Opcode.rs):(__int64)m_Section->MipsRegLo_S(m_Opcode.rs) -
			Is64Bit(m_Opcode.rt)?m_Section->MipsReg(m_Opcode.rt):(__int64)m_Section->MipsRegLo_S(m_Opcode.rt);		
		if (m_Section->MipsRegLo_S(m_Opcode.rd) < 0 && MipsRegHi_S(m_Opcode.rd) == -1){ 
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else if (m_Section->MipsRegLo_S(m_Opcode.rd) >= 0 && MipsRegHi_S(m_Opcode.rd) == 0){ 
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else {
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_64;
		}
	} else {
		if (m_Opcode.rd == m_Opcode.rt) {
			x86Reg HiReg = Map_TempReg(x86_Any,m_Opcode.rt,TRUE);
			x86Reg LoReg = Map_TempReg(x86_Any,m_Opcode.rt,FALSE);
			Map_GPR_64bit(m_Opcode.rd,m_Opcode.rs);
			SubX86RegToX86Reg(cMipsRegLo(m_Opcode.rd),LoReg);
			SbbX86RegToX86Reg(cMipsRegMapHi(m_Opcode.rd),HiReg);
			return;
		}
		Map_GPR_64bit(m_Opcode.rd,m_Opcode.rs);
		if (IsConst(m_Opcode.rt)) {
			SubConstFromX86Reg(cMipsRegLo(m_Opcode.rd),cMipsRegLo(m_Opcode.rt));
			SbbConstFromX86Reg(MipsRegHi(m_Opcode.rd),MipsRegHi(m_Opcode.rt));
		} else if (IsMapped(m_Opcode.rt)) {
			x86Reg HiReg = Is64Bit(m_Opcode.rt)?cMipsRegMapHi(m_Opcode.rt):Map_TempReg(x86_Any,m_Opcode.rt,TRUE);
			ProtectGPR(m_Opcode.rt);
			SubX86RegToX86Reg(cMipsRegLo(m_Opcode.rd),cMipsRegLo(m_Opcode.rt));
			SbbX86RegToX86Reg(cMipsRegMapHi(m_Opcode.rd),HiReg);
		} else {
			SubVariableFromX86reg(cMipsRegLo(m_Opcode.rd),&_GPR[m_Opcode.rt].W[0],CRegName::GPR_Lo[m_Opcode.rt]);
			SbbVariableFromX86reg(MipsRegHi(m_Opcode.rd),&_GPR[m_Opcode.rt].W[1],CRegName::GPR_Hi[m_Opcode.rt]);
		}
	}
#endif
}

void CRecompilerOps::SPECIAL_DSLL (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	if (m_Opcode.rd == 0) { return; }
	if (IsConst(m_Opcode.rt)) {
		if (IsMapped(m_Opcode.rd)) { UnMap_GPR(m_Opcode.rd, FALSE); }

		m_Section->MipsReg(m_Opcode.rd) = Is64Bit(m_Opcode.rt)?m_Section->MipsReg(m_Opcode.rt):(__int64)m_Section->MipsRegLo_S(m_Opcode.rt) << m_Opcode.sa;
		if (m_Section->MipsRegLo_S(m_Opcode.rd) < 0 && MipsRegHi_S(m_Opcode.rd) == -1){ 
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else if (m_Section->MipsRegLo_S(m_Opcode.rd) >= 0 && MipsRegHi_S(m_Opcode.rd) == 0){ 
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else {
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_64;
		}
		return;
	}
	
	Map_GPR_64bit(m_Opcode.rd,m_Opcode.rt);
	ShiftLeftDoubleImmed(MipsRegHi(m_Opcode.rd),cMipsRegLo(m_Opcode.rd),(BYTE)m_Opcode.sa);
	ShiftLeftSignImmed(	cMipsRegLo(m_Opcode.rd),(BYTE)m_Opcode.sa);
#endif
}

void CRecompilerOps::SPECIAL_DSRL (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	if (m_Opcode.rd == 0) { return; }
	if (IsConst(m_Opcode.rt)) {
		if (IsMapped(m_Opcode.rd)) { UnMap_GPR(m_Opcode.rd, FALSE); }

		m_Section->MipsReg(m_Opcode.rd) = Is64Bit(m_Opcode.rt)?m_Section->MipsReg(m_Opcode.rt):(QWORD)m_Section->MipsRegLo_S(m_Opcode.rt) >> m_Opcode.sa;
		if (m_Section->MipsRegLo_S(m_Opcode.rd) < 0 && MipsRegHi_S(m_Opcode.rd) == -1){ 
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else if (m_Section->MipsRegLo_S(m_Opcode.rd) >= 0 && MipsRegHi_S(m_Opcode.rd) == 0){ 
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else {
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_64;
		}
		return;
	}	
	Map_GPR_64bit(m_Opcode.rd,m_Opcode.rt);
	ShiftRightDoubleImmed(cMipsRegLo(m_Opcode.rd),MipsRegHi(m_Opcode.rd),(BYTE)m_Opcode.sa);
	ShiftRightUnsignImmed(MipsRegHi(m_Opcode.rd),(BYTE)m_Opcode.sa);
#endif
}

void CRecompilerOps::SPECIAL_DSRA (void) {
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	if (m_Opcode.rd == 0) { return; }
	if (IsConst(m_Opcode.rt)) {
		if (IsMapped(m_Opcode.rd)) { UnMap_GPR(m_Opcode.rd, FALSE); }

		m_Section->MipsReg_S(m_Opcode.rd) = Is64Bit(m_Opcode.rt)?m_Section->MipsReg_S(m_Opcode.rt):(__int64)m_Section->MipsRegLo_S(m_Opcode.rt) >> m_Opcode.sa;
		if (m_Section->MipsRegLo_S(m_Opcode.rd) < 0 && MipsRegHi_S(m_Opcode.rd) == -1){ 
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else if (m_Section->MipsRegLo_S(m_Opcode.rd) >= 0 && MipsRegHi_S(m_Opcode.rd) == 0){ 
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else {
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_64;
		}
		return;
	}	
	Map_GPR_64bit(m_Opcode.rd,m_Opcode.rt);
	ShiftRightDoubleImmed(cMipsRegMapLo(m_Opcode.rd),cMipsRegMapHi(m_Opcode.rd),(BYTE)m_Opcode.sa);
	ShiftRightSignImmed(cMipsRegMapHi(m_Opcode.rd),(BYTE)m_Opcode.sa);
}

void CRecompilerOps::SPECIAL_DSLL32 (void) {
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	if (m_Opcode.rd == 0) { return; }
	if (IsConst(m_Opcode.rt)) {
		if (m_Opcode.rt != m_Opcode.rd) { UnMap_GPR(m_Opcode.rd, FALSE); }
		MipsRegHi(m_Opcode.rd) = cMipsRegLo(m_Opcode.rt) << m_Opcode.sa;
		MipsRegLo(m_Opcode.rd) = 0;
		if (m_Section->MipsRegLo_S(m_Opcode.rd) < 0 && MipsRegHi_S(m_Opcode.rd) == -1){ 
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else if (m_Section->MipsRegLo_S(m_Opcode.rd) >= 0 && MipsRegHi_S(m_Opcode.rd) == 0){ 
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		} else {
			MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_64;
		}

	} else if (IsMapped(m_Opcode.rt)) {
		ProtectGPR(m_Opcode.rt);
		Map_GPR_64bit(m_Opcode.rd,-1);		
		if (m_Opcode.rt != m_Opcode.rd) {
			MoveX86RegToX86Reg(cMipsRegMapLo(m_Opcode.rt),cMipsRegMapHi(m_Opcode.rd));
		} else {
			CPU_Message("    regcache: switch hi (%s) with lo (%s) for %s",x86_Name(MipsRegMapHi(m_Opcode.rt)),x86_Name(cMipsRegMapLo(m_Opcode.rt)),CRegName::GPR[m_Opcode.rt]);
			x86Reg HiReg = MipsRegMapHi(m_Opcode.rt);
			MipsRegMapHi(m_Opcode.rt) = cMipsRegMapLo(m_Opcode.rt);
			MipsRegMapLo(m_Opcode.rt) = HiReg;
		}
		if ((BYTE)m_Opcode.sa != 0) {
			ShiftLeftSignImmed(cMipsRegMapHi(m_Opcode.rd),(BYTE)m_Opcode.sa);
		}
		XorX86RegToX86Reg(cMipsRegMapLo(m_Opcode.rd),cMipsRegMapLo(m_Opcode.rd));
	} else {
		Map_GPR_64bit(m_Opcode.rd,-1);
		MoveVariableToX86reg(&_GPR[m_Opcode.rt],CRegName::GPR_Hi[m_Opcode.rt],MipsRegMapHi(m_Opcode.rd));
		if ((BYTE)m_Opcode.sa != 0) {
			ShiftLeftSignImmed(MipsRegMapHi(m_Opcode.rd),(BYTE)m_Opcode.sa);
		}
		XorX86RegToX86Reg(cMipsRegMapLo(m_Opcode.rd),cMipsRegMapLo(m_Opcode.rd));
	}
}

void CRecompilerOps::SPECIAL_DSRL32 (void) {
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (IsConst(m_Opcode.rt)) {
		if (m_Opcode.rt != m_Opcode.rd) { UnMap_GPR(m_Opcode.rd, FALSE); }
		MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		MipsRegLo(m_Opcode.rd) = (DWORD)(m_Section->MipsReg(m_Opcode.rt) >> (m_Opcode.sa + 32));
	} else if (IsMapped(m_Opcode.rt)) {
		ProtectGPR(m_Opcode.rt);
		if (Is64Bit(m_Opcode.rt)) {
			if (m_Opcode.rt == m_Opcode.rd) {
				CPU_Message("    regcache: switch hi (%s) with lo (%s) for %s",x86_Name(MipsRegMapHi(m_Opcode.rt)),x86_Name(cMipsRegMapLo(m_Opcode.rt)),CRegName::GPR[m_Opcode.rt]);
				x86Reg HiReg = MipsRegMapHi(m_Opcode.rt);
				MipsRegMapHi(m_Opcode.rt) = cMipsRegMapLo(m_Opcode.rt);
				MipsRegMapLo(m_Opcode.rt) = HiReg;
				Map_GPR_32bit(m_Opcode.rd,FALSE,-1);
			} else {
				Map_GPR_32bit(m_Opcode.rd,FALSE,-1);
				MoveX86RegToX86Reg(MipsRegMapHi(m_Opcode.rt),cMipsRegMapLo(m_Opcode.rd));
			}
			if ((BYTE)m_Opcode.sa != 0) {
				ShiftRightUnsignImmed(cMipsRegMapLo(m_Opcode.rd),(BYTE)m_Opcode.sa);
			}
		} else {
			CRecompilerOps::UnknownOpcode();
		}
	} else {
		Map_GPR_32bit(m_Opcode.rd,FALSE,-1);
		MoveVariableToX86reg(&_GPR[m_Opcode.rt].UW[1],CRegName::GPR_Lo[m_Opcode.rt],cMipsRegMapLo(m_Opcode.rd));
		if ((BYTE)m_Opcode.sa != 0) {
			ShiftRightUnsignImmed(cMipsRegMapLo(m_Opcode.rd),(BYTE)m_Opcode.sa);
		}
	}
}

void CRecompilerOps::SPECIAL_DSRA32 (void) {
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (IsConst(m_Opcode.rt)) {
		if (m_Opcode.rt != m_Opcode.rd) { UnMap_GPR(m_Opcode.rd, FALSE); }
		MipsRegState(m_Opcode.rd) = CRegInfo::STATE_CONST_32;
		MipsRegLo(m_Opcode.rd) = (DWORD)(m_Section->MipsReg_S(m_Opcode.rt) >> (m_Opcode.sa + 32));
	} else if (IsMapped(m_Opcode.rt)) {
		ProtectGPR(m_Opcode.rt);
		if (Is64Bit(m_Opcode.rt)) {
			if (m_Opcode.rt == m_Opcode.rd) {
				CPU_Message("    regcache: switch hi (%s) with lo (%s) for %s",x86_Name(MipsRegMapHi(m_Opcode.rt)),x86_Name(cMipsRegMapLo(m_Opcode.rt)),CRegName::GPR[m_Opcode.rt]);
				x86Reg HiReg = MipsRegMapHi(m_Opcode.rt);
				MipsRegMapHi(m_Opcode.rt) = cMipsRegMapLo(m_Opcode.rt);
				MipsRegMapLo(m_Opcode.rt) = HiReg;
				Map_GPR_32bit(m_Opcode.rd,TRUE,-1);
			} else {
				Map_GPR_32bit(m_Opcode.rd,TRUE,-1);
				MoveX86RegToX86Reg(cMipsRegMapHi(m_Opcode.rt),cMipsRegMapLo(m_Opcode.rd));
			}
			if ((BYTE)m_Opcode.sa != 0) {
				ShiftRightSignImmed(cMipsRegMapLo(m_Opcode.rd),(BYTE)m_Opcode.sa);
			}
		} else {
			CRecompilerOps::UnknownOpcode();
		}
	} else {
		Map_GPR_32bit(m_Opcode.rd,TRUE,-1);
		MoveVariableToX86reg(&_GPR[m_Opcode.rt].UW[1],CRegName::GPR_Lo[m_Opcode.rt],cMipsRegMapLo(m_Opcode.rd));
		if ((BYTE)m_Opcode.sa != 0) {
			ShiftRightSignImmed(cMipsRegMapLo(m_Opcode.rd),(BYTE)m_Opcode.sa);
		}
	}
}

/************************** COP0 functions **************************/
void CRecompilerOps::COP0_MF(void) {
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	switch (m_Opcode.rd) {
	case 9: //Count
		UpdateCounters(m_RegWorkingSet,false,true);
	}
	Map_GPR_32bit(m_Opcode.rt,TRUE,-1);
	MoveVariableToX86reg(&_CP0[m_Opcode.rd],CRegName::Cop0[m_Opcode.rd],cMipsRegMapLo(m_Opcode.rt));
}

void ChangeCompareTimer (void) {
	_SystemTimer->SetTimer(CSystemTimer::CompareTimer, _Reg->COMPARE_REGISTER - _Reg->COUNT_REGISTER,false);
}

void CRecompilerOps::COP0_MT (void) {
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	BYTE *Jump;

	switch (m_Opcode.rd) {
	case 0: //Index
	case 2: //EntryLo0
	case 3: //EntryLo1
	case 4: //Context
	case 5: //PageMask
	case 10: //Entry Hi
	case 14: //EPC
	case 16: //Config
	case 18: //WatchLo 
	case 19: //WatchHi
	case 28: //Tag lo
	case 29: //Tag Hi
	case 30: //ErrEPC
		if (IsConst(m_Opcode.rt)) {
			MoveConstToVariable(cMipsRegLo(m_Opcode.rt), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
		} else if (IsMapped(m_Opcode.rt)) {
			MoveX86regToVariable(cMipsRegMapLo(m_Opcode.rt), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
		} else {
			MoveX86regToVariable(Map_TempReg(x86_Any,m_Opcode.rt,FALSE), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
		}
		if (m_Opcode.rd == 4) //Context
		{
			AndConstToVariable(0xFF800000,&_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
		}
		break;
	case 11: //Compare
		UpdateCounters(m_RegWorkingSet,false,true);
		Pushad();
		MoveConstToX86reg((DWORD)_SystemTimer,x86_ECX);		
		Call_Direct(AddressOf(&CSystemTimer::UpdateTimers), "CSystemTimer::UpdateTimers");
		Popad();
		if (IsConst(m_Opcode.rt)) {
			MoveConstToVariable(cMipsRegLo(m_Opcode.rt), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
		} else if (IsMapped(m_Opcode.rt)) {
			MoveX86regToVariable(cMipsRegMapLo(m_Opcode.rt), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
		} else {
			MoveX86regToVariable(Map_TempReg(x86_Any,m_Opcode.rt,FALSE), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
		}
		AndConstToVariable(~CAUSE_IP7,&_Reg->FAKE_CAUSE_REGISTER,"FAKE_CAUSE_REGISTER");
		Pushad();
		MoveConstToX86reg((DWORD)_SystemTimer,x86_ECX);		
		Call_Direct(AddressOf(&CSystemTimer::UpdateCompareTimer), "CSystemTimer::UpdateCompareTimer");
		Popad();
		break;
	case 9: //Count
		UpdateCounters(m_RegWorkingSet,false,true);
		Pushad();
		MoveConstToX86reg((DWORD)_SystemTimer,x86_ECX);		
		Call_Direct(AddressOf(&CSystemTimer::UpdateTimers), "CSystemTimer::UpdateTimers");
		Popad();
		if (IsConst(m_Opcode.rt)) {
			MoveConstToVariable(cMipsRegLo(m_Opcode.rt), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
		} else if (IsMapped(m_Opcode.rt)) {
			MoveX86regToVariable(cMipsRegMapLo(m_Opcode.rt), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
		} else {
			MoveX86regToVariable(Map_TempReg(x86_Any,m_Opcode.rt,FALSE), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
		}
		Pushad();
		MoveConstToX86reg((DWORD)_SystemTimer,x86_ECX);		
		Call_Direct(AddressOf(&CSystemTimer::UpdateCompareTimer), "CSystemTimer::UpdateCompareTimer");
		Popad();
		break;
	case 12: //Status
		{
			x86Reg OldStatusReg = Map_TempReg(x86_Any,-1,FALSE);
			MoveVariableToX86reg(&_CP0[m_Opcode.rd],CRegName::Cop0[m_Opcode.rd],OldStatusReg);
			if (IsConst(m_Opcode.rt)) {
				MoveConstToVariable(cMipsRegLo(m_Opcode.rt), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
			} else if (IsMapped(m_Opcode.rt)) {
				MoveX86regToVariable(cMipsRegMapLo(m_Opcode.rt), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
			} else {
				MoveX86regToVariable(Map_TempReg(x86_Any,m_Opcode.rt,FALSE), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
			}
			XorVariableToX86reg(&_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd],OldStatusReg);
			TestConstToX86Reg(STATUS_FR,OldStatusReg);
			JeLabel8("FpuFlagFine",0);
			Jump = m_RecompPos - 1;
			Pushad();
			Call_Direct(SetFpuLocations,"SetFpuLocations");
			Popad();
			*(BYTE *)(Jump)= (BYTE )(((BYTE )(m_RecompPos)) - (((BYTE )(Jump)) + 1));
					
			//TestConstToX86Reg(STATUS_FR,OldStatusReg);
			//BreakPoint(__FILE__,__LINE__); //m_Section->CompileExit(m_CompilePC+4,m_RegWorkingSet,ExitResetRecompCode,FALSE,JneLabel32);
			Pushad();
			MoveConstToX86reg((DWORD)_Reg,x86_ECX);
			Call_Direct(AddressOf(&CRegisters::CheckInterrupts),"CRegisters::CheckInterrupts");
			Popad();
		}
		break;
	case 6: //Wired
		_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
		Pushad();
		UpdateCounters(&m_RegWorkingSet.BlockCycleCount(),&m_RegWorkingSet.BlockRandomModifier(),FALSE);
		m_RegWorkingSet.BlockCycleCount() = 0;
		m_RegWorkingSet.BlockRandomModifier() = 0;
		Call_Direct(FixRandomReg,"FixRandomReg");
		Popad();
		if (IsConst(m_Opcode.rt)) {
			MoveConstToVariable(cMipsRegLo(m_Opcode.rt), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
		} else if (IsMapped(m_Opcode.rt)) {
			MoveX86regToVariable(cMipsRegLo(m_Opcode.rt), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
		} else {
			MoveX86regToVariable(Map_TempReg(x86_Any,m_Opcode.rt,FALSE), &_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
		}
#endif
		break;
	case 13: //cause
		if (IsConst(m_Opcode.rt)) {
			AndConstToVariable(0xFFFFCFF,&_CP0[m_Opcode.rd], CRegName::Cop0[m_Opcode.rd]);
#ifndef EXTERNAL_RELEASE
			if ((cMipsRegLo(m_Opcode.rt) & 0x300) != 0 ){ DisplayError("Set IP0 or IP1"); }
#endif
		} else {
			_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
			CRecompilerOps::UnknownOpcode();
#endif
		}
		Pushad();
		MoveConstToX86reg((DWORD)_Reg,x86_ECX);
		Call_Direct(AddressOf(&CRegisters::CheckInterrupts),"CRegisters::CheckInterrupts");
		Popad();
		break;
	default:
		_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
		CRecompilerOps::UnknownOpcode();
#endif
	}
}

/************************** COP0 CO functions ***********************/
void CRecompilerOps::COP0_CO_TLBR( void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (!g_UseTlb) {	return; }
	Pushad();
	Call_Direct(TLB_ReadEntry,"TLB_ReadEntry");
	Popad();
#endif
}

void CRecompilerOps::COP0_CO_TLBWI( void) {
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (!g_UseTlb) {	return; }
	Pushad();
	PushImm32("FALSE",FALSE);
	MoveVariableToX86reg(&_Reg->INDEX_REGISTER,"INDEX_REGISTER",x86_ECX);
	AndConstToX86Reg(x86_ECX,0x1F);
	Push(x86_ECX);
	MoveConstToX86reg((DWORD)_TLB,x86_ECX);
	Call_Direct(AddressOf(&CTLB::WriteEntry),"CTLB::WriteEntry");
	Popad();
}

void CRecompilerOps::COP0_CO_TLBWR( void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	if (!g_UseTlb) {	return; }

	UpdateCounters(&m_RegWorkingSet.BlockCycleCount(),&m_RegWorkingSet.BlockRandomModifier(),FALSE);
	m_RegWorkingSet.BlockCycleCount() = 0;
	m_RegWorkingSet.BlockRandomModifier() = 0;
	Pushad();
	Call_Direct(FixRandomReg,"FixRandomReg");
	PushImm32("TRUE",TRUE);
	MoveVariableToX86reg(&_Reg->RANDOM_REGISTER,"RANDOM_REGISTER",x86_ECX);
	AndConstToX86Reg(x86_ECX,0x1F);
	Push(x86_ECX);
	Call_Direct(TLB_WriteEntry,"TLB_WriteEntry");
	AddConstToX86Reg(x86_ESP,8);
	Popad();
#endif
}

void CRecompilerOps::COP0_CO_TLBP( void) {
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	
	if (!g_UseTlb) {	return; }
	Pushad();
	MoveConstToX86reg((DWORD)_TLB,x86_ECX);		
	Call_Direct(AddressOf(&CTLB::Probe), "CTLB::TLB_Probe");
	Popad();
}

void compiler_COP0_CO_ERET (void) {
	if ((_Reg->STATUS_REGISTER & STATUS_ERL) != 0) {
		_Reg->m_PROGRAM_COUNTER = _Reg->ERROREPC_REGISTER;
		_Reg->STATUS_REGISTER &= ~STATUS_ERL;
	} else {
		_Reg->m_PROGRAM_COUNTER = _Reg->EPC_REGISTER;
		_Reg->STATUS_REGISTER &= ~STATUS_EXL;
	}
	_Reg->m_LLBit = 0;
	_Reg->CheckInterrupts();
}

void CRecompilerOps::COP0_CO_ERET( void) {
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	m_RegWorkingSet.WriteBackRegisters();
	Call_Direct(compiler_COP0_CO_ERET,"compiler_COP0_CO_ERET");
	m_Section->CompileExit(m_CompilePC, (DWORD)-1,m_RegWorkingSet,CExitInfo::Normal,TRUE,NULL);
	m_NextInstruction = END_BLOCK;
}

/************************** FPU Options **************************/
void CRecompilerOps::ChangeDefaultRoundingModel (void) {
	switch((_FPCR[31] & 3)) {
	case 0: *_RoundingModel = ROUND_NEAR; break;
	case 1: *_RoundingModel = ROUND_CHOP; break;
	case 2: *_RoundingModel = ROUND_UP;   break;
	case 3: *_RoundingModel = ROUND_DOWN; break;
	}
}

/************************** COP1 functions **************************/
void CRecompilerOps::COP1_MF (void) {
	x86Reg TempReg;

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	m_Section->CompileCop1Test();

	UnMap_FPR(m_Opcode.fs,TRUE);
	Map_GPR_32bit(m_Opcode.rt, TRUE, -1);
	TempReg = Map_TempReg(x86_Any,-1,FALSE);
	char Name[100];
	sprintf(Name,"_FPR_S[%d]",m_Opcode.fs);
	MoveVariableToX86reg((BYTE *)&_FPR_S[m_Opcode.fs],Name,TempReg);
	MoveX86PointerToX86reg(cMipsRegMapLo(m_Opcode.rt),TempReg);		
}

void CRecompilerOps::COP1_DMF (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	x86Reg TempReg;

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	m_Section->CompileCop1Test();

	UnMap_FPR(m_Opcode.fs,TRUE);
	Map_GPR_64bit(m_Opcode.rt, -1);
	TempReg = Map_TempReg(x86_Any,-1,FALSE);
	sprintf(Name,"_FPRDoubleLocation[%d]",m_Opcode.fs);
	MoveVariableToX86reg((BYTE *)&_FPRDoubleLocation[m_Opcode.fs],Name,TempReg);
	AddConstToX86Reg(TempReg,4);
	MoveX86PointerToX86reg(MipsRegHi(m_Opcode.rt),TempReg);		
	sprintf(Name,"_FPRDoubleLocation[%d]",m_Opcode.fs);
	MoveVariableToX86reg((BYTE *)&_FPRDoubleLocation[m_Opcode.fs],Name,TempReg);
	MoveX86PointerToX86reg(cMipsRegLo(m_Opcode.rt),TempReg);		
#endif
}

void CRecompilerOps::COP1_CF(void) {
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));

	m_Section->CompileCop1Test();
	
	if (m_Opcode.fs != 31 && m_Opcode.fs != 0) { UnknownOpcode(); return; }
	Map_GPR_32bit(m_Opcode.rt,TRUE,-1);
	MoveVariableToX86reg(&_FPCR[m_Opcode.fs],CRegName::FPR_Ctrl[m_Opcode.fs],cMipsRegMapLo(m_Opcode.rt));
}

void CRecompilerOps::COP1_MT( void) {	
	x86Reg TempReg;

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	m_Section->CompileCop1Test();
	
	if ((m_Opcode.fs & 1) != 0) {
		if (RegInStack(m_Opcode.fs-1,CRegInfo::FPU_Double) || RegInStack(m_Opcode.fs-1,CRegInfo::FPU_Qword)) {
			UnMap_FPR(m_Opcode.fs-1,TRUE);
		}
	}
	UnMap_FPR(m_Opcode.fs,TRUE);
	TempReg = Map_TempReg(x86_Any,-1,FALSE);
	char Name[50];
	sprintf(Name,"_FPR_S[%d]",m_Opcode.fs);
	MoveVariableToX86reg((BYTE *)&_FPR_S[m_Opcode.fs],Name,TempReg);

	if (IsConst(m_Opcode.rt)) {
		MoveConstToX86Pointer(cMipsRegLo(m_Opcode.rt),TempReg);
	} else if (IsMapped(m_Opcode.rt)) {
		MoveX86regToX86Pointer(cMipsRegMapLo(m_Opcode.rt),TempReg);
	} else {
		MoveX86regToX86Pointer(Map_TempReg(x86_Any, m_Opcode.rt, FALSE),TempReg);
	}
}

void CRecompilerOps::COP1_DMT( void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	x86Reg TempReg;

	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	m_Section->CompileCop1Test();
	
	if ((m_Opcode.fs & 1) == 0) {
		if (RegInStack(m_Opcode.fs+1,CRegInfo::FPU_Float) || RegInStack(m_Opcode.fs+1,CRegInfo::FPU_Dword)) {
			UnMap_FPR(m_Opcode.fs+1,TRUE);
		}
	}
	UnMap_FPR(m_Opcode.fs,TRUE);
	TempReg = Map_TempReg(x86_Any,-1,FALSE);
	sprintf(Name,"_FPRDoubleLocation[%d]",m_Opcode.fs);
	MoveVariableToX86reg((BYTE *)&_FPRDoubleLocation[m_Opcode.fs],Name,TempReg);
		
	if (IsConst(m_Opcode.rt)) {
		MoveConstToX86Pointer(cMipsRegLo(m_Opcode.rt),TempReg);
		AddConstToX86Reg(TempReg,4);
		if (Section->Is64Bit(m_Opcode.rt)) {
			MoveConstToX86Pointer(MipsRegHi(m_Opcode.rt),TempReg);
		} else {
			MoveConstToX86Pointer(Section->MipsRegLo_S(m_Opcode.rt) >> 31,TempReg);
		}
	} else if (IsMapped(m_Opcode.rt)) {
		MoveX86regToX86Pointer(cMipsRegMapLo(m_Opcode.rt),TempReg);
		AddConstToX86Reg(TempReg,4);
		if (Section->Is64Bit(m_Opcode.rt)) {
			MoveX86regToX86Pointer(MipsRegHi(m_Opcode.rt),TempReg);
		} else {
			MoveX86regToX86Pointer(Map_TempReg(x86_Any, m_Opcode.rt, TRUE),TempReg);
		}
	} else {
		x86Reg Reg= Map_TempReg(x86_Any, m_Opcode.rt, FALSE);
		MoveX86regToX86Pointer(Reg,TempReg);
		AddConstToX86Reg(TempReg,4);
		MoveX86regToX86Pointer(Map_TempReg(Reg, m_Opcode.rt, TRUE),TempReg);
	}
#endif
}


void CRecompilerOps::COP1_CT(void) {
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	m_Section->CompileCop1Test();
	if (m_Opcode.fs != 31) { UnknownOpcode(); return; }

	if (IsConst(m_Opcode.rt)) {
		MoveConstToVariable(cMipsRegLo(m_Opcode.rt),&_FPCR[m_Opcode.fs],CRegName::FPR_Ctrl[m_Opcode.fs]);
	} else if (IsMapped(m_Opcode.rt)) {
		MoveX86regToVariable(cMipsRegMapLo(m_Opcode.rt),&_FPCR[m_Opcode.fs],CRegName::FPR_Ctrl[m_Opcode.fs]);
	} else {
		MoveX86regToVariable(Map_TempReg(x86_Any,m_Opcode.rt,FALSE),&_FPCR[m_Opcode.fs],CRegName::FPR_Ctrl[m_Opcode.fs]);		
	}
	Pushad();
	Call_Direct(ChangeDefaultRoundingModel, "ChangeDefaultRoundingModel");
	Popad();
	m_RegWorkingSet.CurrentRoundingModel() = CRegInfo::RoundUnknown;
}

/************************** COP1: S functions ************************/
void CRecompilerOps::COP1_S_ADD (void) {
	DWORD Reg1 = m_Opcode.ft == m_Opcode.fd?m_Opcode.ft:m_Opcode.fs;
	DWORD Reg2 = m_Opcode.ft == m_Opcode.fd?m_Opcode.fs:m_Opcode.ft;
	
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	
	m_Section->CompileCop1Test();	
	FixRoundModel(CRegInfo::RoundDefault);

	Load_FPR_ToTop(m_Opcode.fd,Reg1, CRegInfo::FPU_Float);
	if (RegInStack(Reg2, CRegInfo::FPU_Float)) {
		fpuAddReg(StackPosition(Reg2));
	} else {
		x86Reg TempReg;

		UnMap_FPR(Reg2,TRUE);
		TempReg = Map_TempReg(x86_Any,-1,FALSE);
		char Name[50];
		sprintf(Name,"_FPR_S[%d]",Reg2);
		MoveVariableToX86reg((BYTE *)&_FPR_S[Reg2],Name,TempReg);
		Load_FPR_ToTop(m_Opcode.fd,m_Opcode.fd, CRegInfo::FPU_Float);
		fpuAddDwordRegPointer(TempReg);
	}
	UnMap_FPR(m_Opcode.fd,TRUE);
}

void CRecompilerOps::COP1_S_SUB (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	DWORD Reg1 = m_Opcode.ft == m_Opcode.fd?m_Opcode.ft:m_Opcode.fs;
	DWORD Reg2 = m_Opcode.ft == m_Opcode.fd?m_Opcode.fs:m_Opcode.ft;
	x86Reg TempReg;
	
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	
	m_Section->CompileCop1Test();	
	FixRoundModel(CRegInfo::RoundDefault);

	if (m_Opcode.fd == m_Opcode.ft) {
		UnMap_FPR(m_Opcode.fd,TRUE);
		Load_FPR_ToTop(m_Opcode.fd,m_Opcode.fs,CRegInfo::FPU_Float);

		TempReg = Map_TempReg(x86_Any,-1,FALSE);
		sprintf(Name,"_FPR_S[%d]",m_Opcode.ft);
		MoveVariableToX86reg((BYTE *)&_FPR_S[m_Opcode.ft],Name,TempReg);
		fpuSubDwordRegPointer(TempReg);
	} else {
		Load_FPR_ToTop(m_Opcode.fd,Reg1, CRegInfo::FPU_Float);
		if (RegInStack(Reg2, CRegInfo::FPU_Float)) {
			fpuSubReg(StackPosition(Reg2));
		} else {
			UnMap_FPR(Reg2,TRUE);
			Load_FPR_ToTop(m_Opcode.fd,m_Opcode.fd, CRegInfo::FPU_Float);

			TempReg = Map_TempReg(x86_Any,-1,FALSE);
			sprintf(Name,"_FPR_S[%d]",Reg2);
			MoveVariableToX86reg((BYTE *)&_FPR_S[Reg2],Name,TempReg);
			fpuSubDwordRegPointer(TempReg);			
		}
	}
	UnMap_FPR(m_Opcode.fd,TRUE);
#endif
}

void CRecompilerOps::COP1_S_MUL (void) {
	DWORD Reg1 = m_Opcode.ft == m_Opcode.fd?m_Opcode.ft:m_Opcode.fs;
	DWORD Reg2 = m_Opcode.ft == m_Opcode.fd?m_Opcode.fs:m_Opcode.ft;
	x86Reg TempReg;
	
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	
	m_Section->CompileCop1Test();	
	FixRoundModel(CRegInfo::RoundDefault);

	Load_FPR_ToTop(m_Opcode.fd,Reg1, CRegInfo::FPU_Float);
	if (RegInStack(Reg2, CRegInfo::FPU_Float)) {
		fpuMulReg(StackPosition(Reg2));
	} else {
		UnMap_FPR(Reg2,TRUE);
		Load_FPR_ToTop(m_Opcode.fd,m_Opcode.fd, CRegInfo::FPU_Float);

		TempReg = Map_TempReg(x86_Any,-1,FALSE);
		char Name[50];
		sprintf(Name,"_FPR_S[%d]",Reg2);
		MoveVariableToX86reg((BYTE *)&_FPR_S[Reg2],Name,TempReg);
		fpuMulDwordRegPointer(TempReg);			
	}
	UnMap_FPR(m_Opcode.fd,TRUE);
}

void CRecompilerOps::COP1_S_DIV (void) {
	DWORD Reg1 = m_Opcode.ft == m_Opcode.fd?m_Opcode.ft:m_Opcode.fs;
	DWORD Reg2 = m_Opcode.ft == m_Opcode.fd?m_Opcode.fs:m_Opcode.ft;
	x86Reg TempReg;
	char Name[50];
	
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	
	m_Section->CompileCop1Test();	
	FixRoundModel(CRegInfo::RoundDefault);

	if (m_Opcode.fd == m_Opcode.ft) {
		UnMap_FPR(m_Opcode.fd,TRUE);
		Load_FPR_ToTop(m_Opcode.fd,m_Opcode.fs,CRegInfo::FPU_Float);

		TempReg = Map_TempReg(x86_Any,-1,FALSE);
		sprintf(Name,"_FPR_S[%d]",m_Opcode.ft);
		MoveVariableToX86reg((BYTE *)&_FPR_S[m_Opcode.ft],Name,TempReg);
		fpuDivDwordRegPointer(TempReg);
	} else {
		Load_FPR_ToTop(m_Opcode.fd,Reg1, CRegInfo::FPU_Float);
		if (RegInStack(Reg2, CRegInfo::FPU_Float)) {
			fpuDivReg(StackPosition(Reg2));
		} else {
			UnMap_FPR(Reg2,TRUE);
			Load_FPR_ToTop(m_Opcode.fd,m_Opcode.fd, CRegInfo::FPU_Float);

			TempReg = Map_TempReg(x86_Any,-1,FALSE);
			sprintf(Name,"_FPR_S[%d]",Reg2);
			MoveVariableToX86reg((BYTE *)&_FPR_S[Reg2],Name,TempReg);
			fpuDivDwordRegPointer(TempReg);			
		}
	}

	UnMap_FPR(m_Opcode.fd,TRUE);
}

void CRecompilerOps::COP1_S_ABS (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	m_Section->CompileCop1Test();	
	FixRoundModel(CRegInfo::RoundDefault);
	Load_FPR_ToTop(m_Opcode.fd,m_Opcode.fs,CRegInfo::FPU_Float);
	fpuAbs();
	UnMap_FPR(m_Opcode.fd,TRUE);
#endif
}

void CRecompilerOps::COP1_S_NEG (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	m_Section->CompileCop1Test();	
	FixRoundModel(CRegInfo::RoundDefault);
	Load_FPR_ToTop(m_Opcode.fd,m_Opcode.fs,CRegInfo::FPU_Float);
	fpuNeg();
	UnMap_FPR(m_Opcode.fd,TRUE);
#endif
}

void CRecompilerOps::COP1_S_SQRT (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	m_Section->CompileCop1Test();	
	FixRoundModel(CRegInfo::RoundDefault);
	Load_FPR_ToTop(m_Opcode.fd,m_Opcode.fs,CRegInfo::FPU_Float);
	fpuSqrt();
	UnMap_FPR(m_Opcode.fd,TRUE);
#endif
}

void CRecompilerOps::COP1_S_MOV (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	m_Section->CompileCop1Test();	
	FixRoundModel(CRegInfo::RoundDefault);
	Load_FPR_ToTop(m_Opcode.fd,m_Opcode.fs,CRegInfo::FPU_Float);
#endif
}

void CRecompilerOps::COP1_S_TRUNC_L (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	
	m_Section->CompileCop1Test();	
	if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd,CRegInfo::FPU_Float)) { 
		Load_FPR_ToTop(m_Opcode.fd,m_Opcode.fs,CRegInfo::FPU_Float); 
	}
	ChangeFPURegFormat(m_Opcode.fd,CRegInfo::FPU_Float,CRegInfo::FPU_Qword,CRegInfo::RoundTruncate);
#endif
}

void CRecompilerOps::COP1_S_CEIL_L (void) {			//added by Witten
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	
	m_Section->CompileCop1Test();	
	if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd,CRegInfo::FPU_Float)) { 
		Load_FPR_ToTop(m_Opcode.fd,m_Opcode.fs,CRegInfo::FPU_Float); 
	}
	ChangeFPURegFormat(m_Opcode.fd,CRegInfo::FPU_Float,CRegInfo::FPU_Qword,CRegInfo::RoundUp);
#endif
}

void CRecompilerOps::COP1_S_FLOOR_L (void) {			//added by Witten
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	
	m_Section->CompileCop1Test();	
	if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd,CRegInfo::FPU_Float)) { 
		Load_FPR_ToTop(m_Opcode.fd,m_Opcode.fs,CRegInfo::FPU_Float); 
	}
	ChangeFPURegFormat(m_Opcode.fd,CRegInfo::FPU_Float,CRegInfo::FPU_Qword,CRegInfo::RoundDown);
#endif
}

void CRecompilerOps::COP1_S_ROUND_W (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	
	m_Section->CompileCop1Test();	
	if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd,CRegInfo::FPU_Float)) { 
		Load_FPR_ToTop(m_Opcode.fd,m_Opcode.fs,CRegInfo::FPU_Float); 
	}
	ChangeFPURegFormat(m_Opcode.fd,CRegInfo::FPU_Float,CRegInfo::FPU_Dword,CRegInfo::RoundNearest);
#endif
}

void CRecompilerOps::COP1_S_TRUNC_W (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	
	m_Section->CompileCop1Test();	
	if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd,CRegInfo::FPU_Float)) { 
		Load_FPR_ToTop(m_Opcode.fd,m_Opcode.fs,CRegInfo::FPU_Float); 
	}
	ChangeFPURegFormat(m_Opcode.fd,CRegInfo::FPU_Float,CRegInfo::FPU_Dword,CRegInfo::RoundTruncate);
#endif
}

void CRecompilerOps::COP1_S_CEIL_W (void) {			// added by Witten
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	
	m_Section->CompileCop1Test();	
	if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd,CRegInfo::FPU_Float)) { 
		Load_FPR_ToTop(m_Opcode.fd,m_Opcode.fs,CRegInfo::FPU_Float); 
	}
	ChangeFPURegFormat(m_Opcode.fd,CRegInfo::FPU_Float,CRegInfo::FPU_Dword,CRegInfo::RoundUp);
#endif
}

void CRecompilerOps::COP1_S_FLOOR_W (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	
	m_Section->CompileCop1Test();	
	if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd,CRegInfo::FPU_Float)) { 
		Load_FPR_ToTop(m_Opcode.fd,m_Opcode.fs,CRegInfo::FPU_Float); 
	}
	ChangeFPURegFormat(m_Opcode.fd,CRegInfo::FPU_Float,CRegInfo::FPU_Dword,CRegInfo::RoundDown);
#endif
}

void CRecompilerOps::COP1_S_CVT_D (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	
	m_Section->CompileCop1Test();	
	if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd,CRegInfo::FPU_Float)) {
		Load_FPR_ToTop(m_Opcode.fd,m_Opcode.fs,CRegInfo::FPU_Float);
	}
	ChangeFPURegFormat(m_Opcode.fd,CRegInfo::FPU_Float,CRegInfo::FPU_Double,CRegInfo::RoundDefault);
#endif
}

void CRecompilerOps::COP1_S_CVT_W (void) {
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	
	m_Section->CompileCop1Test();	
	if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd,CRegInfo::FPU_Float)) {
		Load_FPR_ToTop(m_Opcode.fd,m_Opcode.fs,CRegInfo::FPU_Float);
	}
	ChangeFPURegFormat(m_Opcode.fd,CRegInfo::FPU_Float,CRegInfo::FPU_Dword,CRegInfo::RoundDefault);
}

void CRecompilerOps::COP1_S_CVT_L (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	
	m_Section->CompileCop1Test();	
	if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd,CRegInfo::FPU_Float)) {
		Load_FPR_ToTop(m_Opcode.fd,m_Opcode.fs,CRegInfo::FPU_Float);
	}
	ChangeFPURegFormat(m_Opcode.fd,CRegInfo::FPU_Float,CRegInfo::FPU_Qword,CRegInfo::RoundDefault);
#endif
}

void CRecompilerOps::COP1_S_CMP (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	DWORD Reg1 = RegInStack(m_Opcode.ft, CRegInfo::FPU_Float)?m_Opcode.ft:m_Opcode.fs;
	DWORD Reg2 = RegInStack(m_Opcode.ft, CRegInfo::FPU_Float)?m_Opcode.fs:m_Opcode.ft;
	x86Reg Reg, cmp = 0;

	
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	
	m_Section->CompileCop1Test();	
	//if ((m_Opcode.funct & 1) != 0) { CRecompilerOps::UnknownOpcode(Section); }
	if ((m_Opcode.funct & 2) != 0) { cmp |= 0x4000; }
	if ((m_Opcode.funct & 4) != 0) { cmp |= 0x0100; }
	
	Load_FPR_ToTop(Reg1,Reg1, CRegInfo::FPU_Float);
	Map_TempReg(x86_EAX, 0, FALSE);
	if (RegInStack(Reg2, CRegInfo::FPU_Float)) {
		fpuComReg(StackPosition(Reg2),FALSE);
	} else {
		x86Reg TempReg;

		UnMap_FPR(Reg2,TRUE);
		Load_FPR_ToTop(Reg1,Reg1, CRegInfo::FPU_Float);

		TempReg = Map_TempReg(x86_Any,-1,FALSE);
		sprintf(Name,"_FPR_S[%d]",Reg2);
		MoveVariableToX86reg((BYTE *)&_FPR_S[Reg2],Name,TempReg);
		fpuComDwordRegPointer(TempReg,FALSE);
	}
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	AndConstToVariable(~FPCSR_C, &FSTATUS_REGISTER, "FSTATUS_REGISTER");
#endif
	fpuStoreStatus();
	x86reg = Map_TempReg(x86_Any8Bit, 0, FALSE);
	TestConstToX86Reg(cmp,x86_EAX);	
	Setnz(x86reg);
	
	if (cmp != 0) {
		TestConstToX86Reg(cmp,x86_EAX);	
		Setnz(x86reg);
		
		if ((m_Opcode.funct & 1) != 0) {
			x86Reg Reg2 = Map_TempReg(x86_Any8Bit, 0, FALSE);
			AndConstToX86Reg(x86_EAX, 0x4300);
			CompConstToX86reg(x86_EAX, 0x4300);
			Setz(x86reg2);
	
			OrX86RegToX86Reg(x86reg, x86reg2);
		}
	} else if ((m_Opcode.funct & 1) != 0) {
		AndConstToX86Reg(x86_EAX, 0x4300);
		CompConstToX86reg(x86_EAX, 0x4300);
		Setz(x86reg);
	}
	ShiftLeftSignImmed(x86reg, 23);
	OrX86RegToVariable(&_FPCR[31], "_FPCR[31]", x86reg);
#endif
}

/************************** COP1: D functions ************************/
void CRecompilerOps::COP1_D_ADD (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	DWORD Reg1 = m_Opcode.ft == m_Opcode.fd?m_Opcode.ft:m_Opcode.fs;
	DWORD Reg2 = m_Opcode.ft == m_Opcode.fd?m_Opcode.fs:m_Opcode.ft;
	
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	
	m_Section->CompileCop1Test();	

	Load_FPR_ToTop(m_Opcode.fd,Reg1, CRegInfo::FPU_Double);
	if (RegInStack(Reg2, CRegInfo::FPU_Double)) {
		fpuAddReg(StackPosition(Reg2));
	} else {
		x86Reg TempReg;

		UnMap_FPR(Reg2,TRUE);
		TempReg = Map_TempReg(x86_Any,-1,FALSE);
		sprintf(Name,"_FPRDoubleLocation[%d]",Reg2);
		MoveVariableToX86reg((BYTE *)&_FPRDoubleLocation[Reg2],Name,TempReg);
		Load_FPR_ToTop(m_Opcode.fd,m_Opcode.fd, CRegInfo::FPU_Double);
		fpuAddQwordRegPointer(TempReg);	
	}
#endif
}

void CRecompilerOps::COP1_D_SUB (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	DWORD Reg1 = m_Opcode.ft == m_Opcode.fd?m_Opcode.ft:m_Opcode.fs;
	DWORD Reg2 = m_Opcode.ft == m_Opcode.fd?m_Opcode.fs:m_Opcode.ft;
	x86Reg TempReg;
	
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	
	m_Section->CompileCop1Test();	

	if (m_Opcode.fd == m_Opcode.ft) {
		UnMap_FPR(m_Opcode.fd,TRUE);
		TempReg = Map_TempReg(x86_Any,-1,FALSE);
		sprintf(Name,"_FPRDoubleLocation[%d]",m_Opcode.ft);
		MoveVariableToX86reg((BYTE *)&_FPRDoubleLocation[m_Opcode.ft],Name,TempReg);
		Load_FPR_ToTop(m_Opcode.fd,m_Opcode.fs,CRegInfo::FPU_Double);
		fpuSubQwordRegPointer(TempReg);
	} else {
		Load_FPR_ToTop(m_Opcode.fd,Reg1, CRegInfo::FPU_Double);
		if (RegInStack(Reg2, CRegInfo::FPU_Double)) {
			fpuSubReg(StackPosition(Reg2));
		} else {
			UnMap_FPR(Reg2,TRUE);

			TempReg = Map_TempReg(x86_Any,-1,FALSE);
			sprintf(Name,"_FPRDoubleLocation[%d]",Reg2);
			MoveVariableToX86reg((BYTE *)&_FPRDoubleLocation[Reg2],Name,TempReg);
			Load_FPR_ToTop(m_Opcode.fd,m_Opcode.fd, CRegInfo::FPU_Double);
			fpuSubQwordRegPointer(TempReg);
		}
	}
#endif
}

void CRecompilerOps::COP1_D_MUL (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	DWORD Reg1 = m_Opcode.ft == m_Opcode.fd?m_Opcode.ft:m_Opcode.fs;
	DWORD Reg2 = m_Opcode.ft == m_Opcode.fd?m_Opcode.fs:m_Opcode.ft;
	x86Reg TempReg;
	
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	
	m_Section->CompileCop1Test();	
	FixRoundModel(CRegInfo::RoundDefault);

	Load_FPR_ToTop(m_Opcode.fd,Reg1, CRegInfo::FPU_Double);
	if (RegInStack(Reg2, CRegInfo::FPU_Double)) {
		fpuMulReg(StackPosition(Reg2));
	} else {
		UnMap_FPR(Reg2,TRUE);
		Load_FPR_ToTop(m_Opcode.fd,m_Opcode.fd, CRegInfo::FPU_Double);
		TempReg = Map_TempReg(x86_Any,-1,FALSE);
		sprintf(Name,"_FPRDoubleLocation[%d]",Reg2);
		MoveVariableToX86reg((BYTE *)&_FPRDoubleLocation[Reg2],Name,TempReg);
		fpuMulQwordRegPointer(TempReg);
	}
#endif
}

void CRecompilerOps::COP1_D_DIV (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	DWORD Reg1 = m_Opcode.ft == m_Opcode.fd?m_Opcode.ft:m_Opcode.fs;
	DWORD Reg2 = m_Opcode.ft == m_Opcode.fd?m_Opcode.fs:m_Opcode.ft;
	x86Reg TempReg;
	
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	
	m_Section->CompileCop1Test();	

	if (m_Opcode.fd == m_Opcode.ft) {
		UnMap_FPR(m_Opcode.fd,TRUE);
		TempReg = Map_TempReg(x86_Any,-1,FALSE);
		sprintf(Name,"_FPRDoubleLocation[%d]",m_Opcode.ft);
		MoveVariableToX86reg((BYTE *)&_FPRDoubleLocation[m_Opcode.ft],Name,TempReg);
		Load_FPR_ToTop(m_Opcode.fd,m_Opcode.fs,CRegInfo::FPU_Double);
		fpuDivQwordRegPointer(TempReg);
	} else {
		Load_FPR_ToTop(m_Opcode.fd,Reg1, CRegInfo::FPU_Double);
		if (RegInStack(Reg2, CRegInfo::FPU_Double)) {
			fpuDivReg(StackPosition(Reg2));
		} else {
			UnMap_FPR(Reg2,TRUE);
			TempReg = Map_TempReg(x86_Any,-1,FALSE);
			sprintf(Name,"_FPRDoubleLocation[%d]",Reg2);
			MoveVariableToX86reg((BYTE *)&_FPRDoubleLocation[Reg2],Name,TempReg);
			Load_FPR_ToTop(m_Opcode.fd,m_Opcode.fd, CRegInfo::FPU_Double);
			fpuDivQwordRegPointer(TempReg);
		}
	}
#endif
}

void CRecompilerOps::COP1_D_ABS (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	Load_FPR_ToTop(m_Opcode.fd,m_Opcode.fs,CRegInfo::FPU_Double);
	fpuAbs();
#endif
}

void CRecompilerOps::COP1_D_NEG (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	Load_FPR_ToTop(m_Opcode.fd,m_Opcode.fs,CRegInfo::FPU_Double);
	fpuNeg();
#endif
}

void CRecompilerOps::COP1_D_SQRT (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	Load_FPR_ToTop(m_Opcode.fd,m_Opcode.fs,CRegInfo::FPU_Double);
	fpuSqrt();
#endif
}

void CRecompilerOps::COP1_D_MOV (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	Load_FPR_ToTop(m_Opcode.fd,m_Opcode.fs,CRegInfo::FPU_Double);
#endif
}

void CRecompilerOps::COP1_D_TRUNC_L (void) {			//added by Witten
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	
	m_Section->CompileCop1Test();	
	if (RegInStack(m_Opcode.fs,CRegInfo::FPU_Double) || RegInStack(m_Opcode.fs,CRegInfo::FPU_Qword)) {
		UnMap_FPR(m_Opcode.fs,TRUE);
	}
	if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd,CRegInfo::FPU_Double)) { 
		Load_FPR_ToTop(m_Opcode.fd,m_Opcode.fs,CRegInfo::FPU_Double); 
	}
	ChangeFPURegFormat(m_Opcode.fd,CRegInfo::FPU_Double,CRegInfo::FPU_Qword,CRegInfo::RoundTruncate);
#endif
}

void CRecompilerOps::COP1_D_CEIL_L (void) {			//added by Witten
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	
	m_Section->CompileCop1Test();	
	if (RegInStack(m_Opcode.fs,CRegInfo::FPU_Double) || RegInStack(m_Opcode.fs,CRegInfo::FPU_Qword)) {
		UnMap_FPR(m_Opcode.fs,TRUE);
	}
	if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd,CRegInfo::FPU_Double)) { 
		Load_FPR_ToTop(m_Opcode.fd,m_Opcode.fs,CRegInfo::FPU_Double); 
	}
	ChangeFPURegFormat(m_Opcode.fd,CRegInfo::FPU_Double,CRegInfo::FPU_Qword,CRegInfo::RoundUp);
#endif
}

void CRecompilerOps::COP1_D_FLOOR_L (void) {			//added by Witten
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	
	m_Section->CompileCop1Test();	
	if (RegInStack(m_Opcode.fs,CRegInfo::FPU_Double) || RegInStack(m_Opcode.fs,CRegInfo::FPU_Qword)) {
		UnMap_FPR(m_Opcode.fs,TRUE);
	}
	if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd,CRegInfo::FPU_Double)) { 
		Load_FPR_ToTop(m_Opcode.fd,m_Opcode.fs,CRegInfo::FPU_Double); 
	}
	ChangeFPURegFormat(m_Opcode.fd,CRegInfo::FPU_Double,CRegInfo::FPU_Qword,CRegInfo::RoundDown);
#endif
}

void CRecompilerOps::COP1_D_ROUND_W (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	
	m_Section->CompileCop1Test();	
	if (RegInStack(m_Opcode.fs,CRegInfo::FPU_Double) || RegInStack(m_Opcode.fs,CRegInfo::FPU_Qword)) {
		UnMap_FPR(m_Opcode.fs,TRUE);
	}
	if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd,CRegInfo::FPU_Double)) { 
		Load_FPR_ToTop(m_Opcode.fd,m_Opcode.fs,CRegInfo::FPU_Double); 
	}
	ChangeFPURegFormat(m_Opcode.fd,CRegInfo::FPU_Double,CRegInfo::FPU_Dword,CRegInfo::RoundNearest);
#endif
}

void CRecompilerOps::COP1_D_TRUNC_W (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	
	m_Section->CompileCop1Test();	
	if (RegInStack(m_Opcode.fs,CRegInfo::FPU_Double) || RegInStack(m_Opcode.fs,CRegInfo::FPU_Qword)) {
		UnMap_FPR(m_Opcode.fs,TRUE);
	}
	if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd,CRegInfo::FPU_Double)) { 
		Load_FPR_ToTop(m_Opcode.fd,m_Opcode.fs,CRegInfo::FPU_Double); 
	}
	ChangeFPURegFormat(m_Opcode.fd,CRegInfo::FPU_Double,CRegInfo::FPU_Dword,CRegInfo::RoundTruncate);
#endif
}

void CRecompilerOps::COP1_D_CEIL_W (void) {				// added by Witten
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	
	m_Section->CompileCop1Test();	
	if (RegInStack(m_Opcode.fs,CRegInfo::FPU_Double) || RegInStack(m_Opcode.fs,CRegInfo::FPU_Qword)) {
		UnMap_FPR(m_Opcode.fs,TRUE);
	}
	if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd,CRegInfo::FPU_Double)) { 
		Load_FPR_ToTop(m_Opcode.fd,m_Opcode.fs,CRegInfo::FPU_Double); 
	}
	ChangeFPURegFormat(m_Opcode.fd,CRegInfo::FPU_Double,CRegInfo::FPU_Dword,CRegInfo::RoundUp);
#endif
}

void CRecompilerOps::COP1_D_FLOOR_W (void) {			//added by Witten
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	
	m_Section->CompileCop1Test();	
	if (RegInStack(m_Opcode.fs,CRegInfo::FPU_Double) || RegInStack(m_Opcode.fs,CRegInfo::FPU_Qword)) {
		UnMap_FPR(m_Opcode.fs,TRUE);
	}
	if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd,CRegInfo::FPU_Double)) { 
		Load_FPR_ToTop(m_Opcode.fd,m_Opcode.fs,CRegInfo::FPU_Double); 
	}
	ChangeFPURegFormat(m_Opcode.fd,CRegInfo::FPU_Double,CRegInfo::FPU_Dword,CRegInfo::RoundDown);
#endif
}

void CRecompilerOps::COP1_D_CVT_S (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	
	m_Section->CompileCop1Test();	
	if (RegInStack(m_Opcode.fs,CRegInfo::FPU_Double) || RegInStack(m_Opcode.fs,CRegInfo::FPU_Qword)) {
		UnMap_FPR(m_Opcode.fs,TRUE);
	}
	if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd,CRegInfo::FPU_Double)) { 
		Load_FPR_ToTop(m_Opcode.fd,m_Opcode.fs,CRegInfo::FPU_Double); 
	}
	ChangeFPURegFormat(m_Opcode.fd,CRegInfo::FPU_Double,CRegInfo::FPU_Float,CRegInfo::RoundDefault);
#endif
}

void CRecompilerOps::COP1_D_CVT_W (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	
	m_Section->CompileCop1Test();	
	if (RegInStack(m_Opcode.fs,CRegInfo::FPU_Double) || RegInStack(m_Opcode.fs,CRegInfo::FPU_Qword)) {
		UnMap_FPR(m_Opcode.fs,TRUE);
	}
	if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd,CRegInfo::FPU_Double)) { 
		Load_FPR_ToTop(m_Opcode.fd,m_Opcode.fs,CRegInfo::FPU_Double); 
	}
	ChangeFPURegFormat(m_Opcode.fd,CRegInfo::FPU_Double,CRegInfo::FPU_Dword,CRegInfo::RoundDefault);
#endif
}

void CRecompilerOps::COP1_D_CVT_L (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	
	m_Section->CompileCop1Test();	
	if (RegInStack(m_Opcode.fs,CRegInfo::FPU_Double) || RegInStack(m_Opcode.fs,CRegInfo::FPU_Qword)) {
		UnMap_FPR(m_Opcode.fs,TRUE);
	}
	if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd,CRegInfo::FPU_Double)) { 
		Load_FPR_ToTop(m_Opcode.fd,m_Opcode.fs,CRegInfo::FPU_Double); 
	}
	ChangeFPURegFormat(m_Opcode.fd,CRegInfo::FPU_Double,CRegInfo::FPU_Qword,CRegInfo::RoundDefault);
#endif
}

void CRecompilerOps::COP1_D_CMP (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	DWORD Reg1 = RegInStack(m_Opcode.ft, CRegInfo::FPU_Float)?m_Opcode.ft:m_Opcode.fs;
	DWORD Reg2 = RegInStack(m_Opcode.ft, CRegInfo::FPU_Float)?m_Opcode.fs:m_Opcode.ft;
	x86Reg Reg, cmp = 0;

	
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	
	m_Section->CompileCop1Test();	
	//if ((m_Opcode.funct & 1) != 0) { CRecompilerOps::UnknownOpcode(Section); }
	if ((m_Opcode.funct & 2) != 0) { cmp |= 0x4000; }
	if ((m_Opcode.funct & 4) != 0) { cmp |= 0x0100; }
	
	Load_FPR_ToTop(Reg1,Reg1, CRegInfo::FPU_Double);
	Map_TempReg(x86_EAX, 0, FALSE);
	if (RegInStack(Reg2, CRegInfo::FPU_Double)) {
		fpuComReg(StackPosition(Reg2),FALSE);
	} else {
		x86Reg TempReg;

		UnMap_FPR(Reg2,TRUE);
		TempReg = Map_TempReg(x86_Any,-1,FALSE);
		sprintf(Name,"_FPRDoubleLocation[%d]",Reg2);
		MoveVariableToX86reg((BYTE *)&_FPRDoubleLocation[Reg2],Name,TempReg);
		Load_FPR_ToTop(Reg1,Reg1, CRegInfo::FPU_Double);
		fpuComQwordRegPointer(TempReg,FALSE);
	}
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	AndConstToVariable(~FPCSR_C, &FSTATUS_REGISTER, "FSTATUS_REGISTER");
#endif
	fpuStoreStatus();
	x86reg = Map_TempReg(x86_Any8Bit, 0, FALSE);
	TestConstToX86Reg(cmp,x86_EAX);	
	Setnz(x86reg);
	if (cmp != 0) {
		TestConstToX86Reg(cmp,x86_EAX);	
		Setnz(x86reg);
		
		if ((m_Opcode.funct & 1) != 0) {
			x86Reg Reg2 = Map_TempReg(x86_Any8Bit, 0, FALSE);
			AndConstToX86Reg(x86_EAX, 0x4300);
			CompConstToX86reg(x86_EAX, 0x4300);
			Setz(x86reg2);
	
			OrX86RegToX86Reg(x86reg, x86reg2);
		}
	} else if ((m_Opcode.funct & 1) != 0) {
		AndConstToX86Reg(x86_EAX, 0x4300);
		CompConstToX86reg(x86_EAX, 0x4300);
		Setz(x86reg);
	}
	ShiftLeftSignImmed(x86reg, 23);
	OrX86RegToVariable(&_FPCR[31], "_FPCR[31]", x86reg);
#endif
}

/************************** COP1: W functions ************************/
void CRecompilerOps::COP1_W_CVT_S (void) {
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	
	m_Section->CompileCop1Test();	
	if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd,CRegInfo::FPU_Dword)) { 
		Load_FPR_ToTop(m_Opcode.fd,m_Opcode.fs,CRegInfo::FPU_Dword); 
	}
	ChangeFPURegFormat(m_Opcode.fd,CRegInfo::FPU_Dword,CRegInfo::FPU_Float,CRegInfo::RoundDefault);
}

void CRecompilerOps::COP1_W_CVT_D (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	
	m_Section->CompileCop1Test();	
	if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd,CRegInfo::FPU_Dword)) { 
		Load_FPR_ToTop(m_Opcode.fd,m_Opcode.fs,CRegInfo::FPU_Dword); 
	}
	ChangeFPURegFormat(m_Opcode.fd,CRegInfo::FPU_Dword,CRegInfo::FPU_Double,CRegInfo::RoundDefault);
#endif
}

/************************** COP1: L functions ************************/
void CRecompilerOps::COP1_L_CVT_S (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	
	m_Section->CompileCop1Test();	
	if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd,CRegInfo::FPU_Qword)) { 
		Load_FPR_ToTop(m_Opcode.fd,m_Opcode.fs,CRegInfo::FPU_Qword); 
	}
	ChangeFPURegFormat(m_Opcode.fd,CRegInfo::FPU_Qword,CRegInfo::FPU_Float,CRegInfo::RoundDefault);
#endif
}

void CRecompilerOps::COP1_L_CVT_D (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X %s",m_CompilePC,R4300iOpcodeName(m_Opcode.Hex,m_CompilePC));
	
	m_Section->CompileCop1Test();	
	if (m_Opcode.fd != m_Opcode.fs || !RegInStack(m_Opcode.fd,CRegInfo::FPU_Qword)) { 
		Load_FPR_ToTop(m_Opcode.fd,m_Opcode.fs,CRegInfo::FPU_Qword); 
	}
	ChangeFPURegFormat(m_Opcode.fd,CRegInfo::FPU_Qword,CRegInfo::FPU_Double,CRegInfo::RoundDefault);
#endif
}

/************************** Other functions **************************/
void CRecompilerOps::UnknownOpcode (void) {
	_Notify->BreakPoint(__FILE__,__LINE__);
#ifdef tofix
	CPU_Message("  %X Unhandled Opcode: %s",m_CompilePC, R4300iOpcodeName(Opcode.Hex,m_CompilePC));

//	FreeSection(m_Section->m_ContinueSection,m_Section);
//	FreeSection(m_Section->m_JumpSection,m_Section);
	m_RegWorkingSet.BlockCycleCount() -= CountPerOp;
	m_RegWorkingSet.BlockRandomModifier() -= 1;
	MoveConstToVariable(m_CompilePC,_PROGRAM_COUNTER,"PROGRAM_COUNTER");
	m_RegWorkingSet.WriteBackRegisters();
	UpdateCounters(&m_RegWorkingSet.BlockCycleCount(),&m_RegWorkingSet.BlockRandomModifier(),FALSE);
	if (CPU_Type == CPU_SyncCores) { Call_Direct(SyncToPC, "SyncToPC"); }
	MoveConstToVariable(Opcode.Hex,&Opcode.Hex,"Opcode.Hex");
	Call_Direct(R4300iOp::UnknownOpcode, "R4300iOp::UnknownOpcode");
	Ret();
	if (m_NextInstruction == NORMAL) { m_NextInstruction = END_BLOCK; }
#endif
}

void CRecompilerOps::EnterCodeBlock ( void )
{
#ifdef _DEBUG
	Push(x86_ESI);
#endif
}

void CRecompilerOps::ExitCodeBlock ( void )
{
#ifdef _DEBUG
	Pop(x86_ESI);
#endif
	Ret();
}

void CRecompilerOps::UpdateCounters ( CRegInfo & RegSet, bool CheckTimer, bool ClearValues )
{
	if (RegSet.GetBlockCycleCount() != 0)
	{
		if (_SyncSystem) {

			WriteX86Comment("Updating Sync CPU");
			Pushad();
			PushImm32(stdstr_f("%d",RegSet.GetBlockCycleCount()).c_str(),RegSet.GetBlockCycleCount());
			PushImm32("_SyncSystem",(DWORD)_SyncSystem);
			MoveConstToX86reg((DWORD)_N64System,x86_ECX);		
			Call_Direct(AddressOf(&CN64System::UpdateSyncCPU),"CN64System::UpdateSyncCPU");
			Popad();
		}
		WriteX86Comment("Update Counter");
		SubConstFromVariable(RegSet.GetBlockCycleCount(),_NextTimer,"_NextTimer"); // updates compare flag
		if (ClearValues)
		{
			RegSet.SetBlockCycleCount(0);
		}
	} else if (CheckTimer)	{
		CompConstToVariable(0,_NextTimer,"_NextTimer");
	}

	if (CheckTimer)
	{
		JnsLabel8("Continue_From_Timer_Test",0);
		BYTE * Jump = m_RecompPos - 1;
		Pushad();
		MoveConstToX86reg((DWORD)_SystemTimer,x86_ECX);		
		Call_Direct(AddressOf(&CSystemTimer::TimerDone),"CSystemTimer::TimerDone");
		Popad();
	
		CPU_Message("");
		CPU_Message("      $Continue_From_Timer_Test:");
		SetJump8(Jump,m_RecompPos);
	}
}

