package vexriscv.demo

import spinal.core._
import vexriscv.ip.{DataCacheConfig, InstructionCacheConfig}
import vexriscv.plugin._
import vexriscv.{VexRiscv, VexRiscvConfig, plugin}

/**
 * Created by spinalvm on 15.06.17.
 */
object GenerateControlHistoricalGeomImcExP1 extends App{
  def cpu() = new VexRiscv(
    config = VexRiscvConfig(
      plugins = List(
        new PcManagerSimplePlugin(
          resetVector = 0x00000000l,
          relaxedPcCalculation = false
        ),
        new IBusCachedPlugin(
          resetVector = 0x00000000l,
          prediction = DYNAMIC_TARGET,
          compressedGen = true,
          injectorStage = true,
          monitorGen = true,
          historyRamSizeLog2 = 8,
          config = InstructionCacheConfig(
            cacheSize = 16384,
            bytePerLine =16,
            wayCount = 2,
            addressWidth = 32,
            cpuDataWidth = 32,
            memDataWidth = 32,
            catchIllegalAccess = true,
            catchAccessFault = true,
            asyncTagMemory = false,
            twoCycleRam = false,
            twoCycleCache = true
          )
        ),
        new DBusCachedPlugin(
          config = new DataCacheConfig(
            cacheSize         = 2048,
            bytePerLine       = 16,
            wayCount          = 2,
            addressWidth      = 32,
            cpuDataWidth      = 32,
            memDataWidth      = 32,
            catchAccessError  = true,
            catchIllegal      = true,
            catchUnaligned    = true
          )
        ),
        new StaticMemoryTranslatorPlugin(
          ioRange      = _(31 downto 16) =/= 0
        ),
        new DecoderSimplePlugin(
          catchIllegalInstruction = true
        ),
        new RegFilePlugin(
          regFileReadyKind = plugin.SYNC,
          zeroBoot = false
        ),
        new IntAluPlugin,
        new SrcPlugin(
          separatedAddSub = false,
          executeInsertion = true
        ),
        new FullBarrelShifterPlugin(earlyInjection = true),
        new HazardSimplePlugin(
          bypassExecute           = true,
          bypassMemory            = true,
          bypassWriteBack         = true,
          bypassWriteBackBuffer   = true,
          pessimisticUseSrc       = false,
          pessimisticWriteRegFile = false,
          pessimisticAddressMatch = false
        ),
        new MulPlugin,
        new DivPlugin,
        new CsrPlugin(CsrPluginConfig(
          catchIllegalAccess = true,
          mvendorid = 0,
          marchid = 0,
          mimpid = 0,
          mhartid = 0,
          misaExtensionsInit = 4356,
          misaAccess = CsrAccess.READ_ONLY,
          mtvecAccess = CsrAccess.READ_WRITE,
          mtvecInit = 0,
          mepcAccess = CsrAccess.READ_WRITE,
          mscratchGen = true,
          mcauseAccess = CsrAccess.READ_WRITE,
          mbadaddrAccess = CsrAccess.READ_WRITE,
          mcycleAccess = CsrAccess.READ_WRITE,
          minstretAccess = CsrAccess.READ_WRITE,
          ucycleAccess = CsrAccess.READ_ONLY,
          uinstretAccess = CsrAccess.READ_ONLY,
          utimeAccess = CsrAccess.READ_ONLY,
          wfiGenAsWait = false,
          wfiGenAsNop = true,
          ecallGen = true,
          ebreakGen = true,
          xtvecModeGen = true,
          userGen = false,
          supervisorGen = false,
          h3Matched = true
        )),
        new DebugPlugin(ClockDomain.current.clone(reset = Bool().setName("debugReset"))),
        new BranchPlugin(
          earlyBranch = true,
          catchAddressMisaligned = true
        ),
        new PangoCachedTraceMemory,
        new FormalPlugin,
        new PangoNativeDataMonitor,
        new PangoBranchOwnerTrace,
        new YamlPlugin("D:/AAAFPGA/ZGTC_RISC_CPU/cpu_paper/ccfa-workfiles/novelty-enhancement/vex/implementation/generation/historical_geom_imc_ex_p1/cpu0.yaml")
      )
    )
  )

  SpinalConfig(targetDirectory = "D:/AAAFPGA/ZGTC_RISC_CPU/cpu_paper/ccfa-workfiles/novelty-enhancement/vex/implementation/generation/historical_geom_imc_ex_p1").generateVerilog(cpu())
}
