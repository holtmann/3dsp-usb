/************************************************************************
(c) 2003-04, 3DSP Corporation, all rights reserved.  Duplication or
reproduction of any part of this software (source code, object code or
comments) without the expressed written consent by 3DSP Corporation is
forbidden.  For further information please contact:

3DSP Corporation
16271 Laguna Canyon Rd
Irvine, CA 92618
www.3dsp.com
 **************************************************************************
$RCSfile: bt_version.h,v $
$Revision: 1.16 $
$Date: 2010/12/10 02:13:13 $
 **************************************************************************/
/*
 * description:
 * 	This Header file, contains stack version information
 *
 */

/*
*ChangeLog:
------------------------------------------------------------------------
<Jakio.20080218>:
1. Release connection when suprised removing card, otherwise, the ivt programme can't
	be closed.
2. Fix some combo state bug, these bugs were introduced in last version when fixing
	bugs of power management. These bugs will disorder the state of driver when switch
	by wb.
------------------------------------------------------------------------
<20080219>:
1. query space when sending lmp and poll failure(when failure reason is not enough space)
2. del an assert, and add some code to init sco buffer in routine 'Frag_InitTxScoForConnDev'
3. combin the changed afh code, now the afh feature is supported.
------------------------------------------------------------------------
<20080221>
1. add an ioctl code to report driver state to WB
2. improve function of auto packet selecting 
3. add macro 'BT_MATCH_CLASS_OF_DEVICE', this is introduced to solve bugs when connecting
	multi devices,but now the bug is not completely fixed.
4. improve flow of role-switch failure
5. open 3dh5 feature
------------------------------------------------------------------------
<20080223>
1. Judge driver state in 'BtWatchdogItem', if not in STATE_STARTED,we should return immediately.
	otherwise,this will lead to a BSD in multi-processor computer when suprised removing card.
2. improve the flow of entering and leaving sniff mode. but sniff feature now is not supported.
3. fix a bug of query space infomation. This will lead to failure when 'sony errisson810' connect 
	our card.
------------------------------------------------------------------------
<20080223_PM_1400>
1. clear sco tx buffer when delete sco link.
2. return from task_threadroutine when the thread is terminated, otherwise, this is not safe
	in multi-processors system
------------------------------------------------------------------------
<20080225>
1. Change the order of release resource, now we release task resource first, this will help
	resolve the bugs of suprised removing card in multi-processor system
------------------------------------------------------------------------
<20080226_TEMP>
1. fix a bug of power management. this is a misplay when combining different driver version
2. add ioctrl to support WB query driver status
------------------------------------------------------------------------
<20080226_FINAL>
1. fix the bug which leads to system hang in multi-processor system
2. increase rx cach number
3. modify the process flow of poll packet, now we only put fhs packets into task. This will be
	helpful to resolve the bug of accepting connectiong repeatedly as slave
------------------------------------------------------------------------
<20080227>
1. Negociate with 8051 when doing combo state switch, this will help avoid wlan and bt 
	operating DSP meanwhile
2. Change set afh flow. send lmp pdu as soon as driver receives clock ready interrupt in 
	dispatch level
------------------------------------------------------------------------
<20080228>
1. delete some code about rx cach management. It is testified useless and can lead BSD 
	in multi-processor system.
2. Enlarge rx cach number from 16 to 32
3. Make sure AFH pdu has been put into DSP buffer before write afh command indicator, this
	is used to solve afh bug(writing afh cmd indicator too early leads to noconsistent channel
	map between several devices, thus all links will be disconnected).
------------------------------------------------------------------------
<20080228_FINAL>
1. Enlarge rx buffer from 16 to 32
2. Roll back rx process policy: when fhs recept, we put subsequent packets(including this one)
	into task. we count on the enlarged rx buffer to avoid the third bug in '20080226_FINAL' until
	we find a better way.
------------------------------------------------------------------------
<20080229_1400>
1. Judge driver status more strictly, taking power management status into account. This is
	for the sake of combo state switch
2. Resolve an AFH bug, prevent array overflow. Sometimes, DSP reports wrong channel frequency,
	we need do some safe check in this case
3. Judge driver state before access hardware, this may helpful to resolve the bug when suprised
	remove card in multi-processor system
------------------------------------------------------------------------
<20080229_FINAL>
1. Fix a bug which is introduced in version '20080229_1400', this can lead to failure when 
	switch combo state from bt only to wlan only
------------------------------------------------------------------------
<20080303_1500>
1. Realize new plan about rx pre_process.
------------------------------------------------------------------------
<20080304>
1. consummate the rx state machine. Prevent changing from RX_STATE_CONNECTING to 
	RX_STATE_CONNECTED too early. This may enhancing the success ratio of slave link
	creation.
------------------------------------------------------------------------
<20080304_FINAL>
1. fix a bug introduced in last version '20080304', there is a deadlock when switch rx state
	machine.
2. when slave connecting, if there is still a link with it, ignore the connecting request and 
	clear slave connection registers.
------------------------------------------------------------------------
<20080305>
1. Add macro define 'BT_ROLESWITCH_UNALLOWED_WHEN_MULTI_SLAVE'
	this prevent network topology changing to master-slave-coexist
------------------------------------------------------------------------
<20080305_FINAL>
1. Dropping bulkin pipe2 irp to wait inquiry result, which changes into an asynchous process
2. add remedy meathod when write dma length failure
3. make sure create task for afh command is success
4. keep  rx state machine as RX_STATE_CONNECTING another 2 seconds after 'link setup comlete'
5. drop interrupt irp after download 8051 when entering powerstate D0
------------------------------------------------------------------------
<20080306>
1. judge whether pci is ready when entering powerstate D0. 
2. cancel tx timer when unplugged
------------------------------------------------------------------------
<20080307>
1. Add opertion to cancel write irp
2. allocate resouces when driver is loading
------------------------------------------------------------------------
<20080307_FINAL>
1. fix a bug when get version from 8051
------------------------------------------------------------------------
<20080311>
1. Fix a bug when asynchronous burst writing registers, it is not safe in multi-processor system
2. Fix a bug when accept sco link request, should set max slots for all devices
------------------------------------------------------------------------
<20080311_FINAL>
1. Protect asynchronous queues by spinlock, avoid destroying data structrue
------------------------------------------------------------------------
<20080314>
1. Cancel DelayTask timer when unplugged
2. serialize some DPC routine, search 'Serialize_DPC_Lock' for details
3. fix a bug of setting DMA length
4. replace the "assert" with debugPrint
5. Check sco data length before sending to DSP,make sure it is four bytes aligned, otherwise DSP
	will be dead
6. check read irp failure count, clear the failure count when driver in idle state.
------------------------------------------------------------------------
<20080318>
1. Fix a bug when check write irp timeout: should considering the situation of download FW
2. Fix a bug of asynochous failure 
3. Fix a bug of 'link error' in free build environment
------------------------------------------------------------------------
<20080320>
1. wait event until timeout when asynchonous sending irp
------------------------------------------------------------------------
<20080324>
1. change slave space size
------------------------------------------------------------------------
<20080327>
1. add subcmd when request 8051 entering mini loop to distinct driver unload from entering
	power save mode
2. fix a bug of low throughput,refer routine 'Frag_TxTimerInstant()'.
------------------------------------------------------------------------
<20080328>
1. fix a mistake of 'c' lanuage syntaxt
2. delete the failure count limit of query space info
------------------------------------------------------------------------
<20080328>
1. fix a bug of setting DMA failure which will lead to BSD
------------------------------------------------------------------------
<20080408>
1. Synchronize with WLAN driver when comeback from power D3, prevent competition between
	WLAN driver and BT driver
------------------------------------------------------------------------
<20080414>
1. there is a mistake of delay time in btpower_setpowerd0(). the timeout is too long,and 
	this can lead to missing chance to communicate with IVT
------------------------------------------------------------------------
<20080416>
1. Add sub command when requesting join
2. Fix a bug when recovering from D3, should request 8051 goto MainLoop
------------------------------------------------------------------------
<20080421>
1. Adjust the code order when requesting 8051 goto mainloop, Makesure this won't conflit
	with wlan driver in combo mode.
------------------------------------------------------------------------
<20080422>
1. Negotiate with 8051 about combo switch, ignore wlan join request when bt is doing swtich
2. Change function of probe 8051 cmd register, judge our card's status when read cmd
	register, this can make driver unload more fluently
------------------------------------------------------------------------
<20080514>
1. change proto type of some routine: UsbCancelRxReq(), UsbCancelInquiryIrp(), 
	UsbCancelInterruptReq(). This can avoid too much delay in driver when combo 
	switch and driver unload.
2. Fix a bug of combo switch: when wlan hardware reset occurs too frequently, bt driver 
	may send unwanted "join" cmd to 8051, thus make 8051 out of order
------------------------------------------------------------------------
<20080516>
1. Inform 8051 goto mainloop after we have done the initialization
2. Delete some unuseful debug print
------------------------------------------------------------------------
<20080517>
1. backup and restore bluetooth registers when when combo switch
------------------------------------------------------------------------
<20080519>
1. Cancel int irp when wlan halt
------------------------------------------------------------------------
<20080519>
1. Adjust code order in btpower_setpowerd0(), where we make informing 8051 goto mainloop
	to be the last thing
------------------------------------------------------------------------
<20080520>
1. Add code to handle mailbox cmd lost of "request 8051 mainloop", now the code is closed,
	if 8051 can't spatch it, we can handle it in driver by opening this code 
------------------------------------------------------------------------
<20080521>
1. Download 8051 FW twice when restore from S3 or hibernation, this can resolve hibernation
	bug of BT only
------------------------------------------------------------------------
<20080522>
1. Change Slave and sco fifo size
------------------------------------------------------------------------
<20080523>
1. Fix a bug of SCO connection in combo state, we should init some vars in combo state,
	refer for "Send_SCOLinkReq_PDU" for detail.
	Add two functions:Hci_ComboStateInit(),Hci_SingleStateInit(). when combo state
	switch occurs, should init the special vars to keeps with pci projects
------------------------------------------------------------------------
<20080523>
1. open power ctrl support in combo mode.
------------------------------------------------------------------------
<20080526>
1. Check if 8051 has completed initaization before requesting join, otherwise the cmd will
	be lost
2. delete some unuseful debug print message.
------------------------------------------------------------------------
<20080527>
1.Write inquiry length right from IVT, which is half of the value we use in combo mode 
2. Add inquiry protect timeout function
3. Don't change driver state in int completion, otherwise the flow in pnp handler will be wrong
	which would make driver  unloadable.
------------------------------------------------------------------------
<20080528>
1. If urb is pending in usbccgp.sys, don't free the urb and make driver halt to avoid BSD
2. Add some debug print when adding sco link
------------------------------------------------------------------------
<20080529>
1. judge device state when waiting 8051 completing initialization,this can avoid driver can't
	be unloaded
------------------------------------------------------------------------
<20080530>
1. When driver detect device's removed state, driver should report to WB, then WB will
	close IVT and its service, otherwise, driver may not receive remove cmd from pnp
	manager.
2. Add time delay when there are too may retries of reading/writing usb mailbox, this can
	avoid access reg timeout.
------------------------------------------------------------------------
<20080530>
1. Retry "request 8051 goto mainloop" command in btpower_setpowerd0(). Because this 
	cmd may lost sometimes
------------------------------------------------------------------------
<200800610>
1. Patch for vista. On some vista system, vcmd of query dma space losts sometimes, we
	take some remedial measures to resolve this
------------------------------------------------------------------------
<200800612>
1. After restore from S3, delay a while before read PCI ready bit, HW need time to complete
	initialization.
2. Patch for vista. when halt or suprise remove card, the status of returned irp is different
	from XP(in XP, status is:0xc000009d, in vista is:0xc000000e).
3. judge sco device when build sco frag, make sure there is not null pointer
------------------------------------------------------------------------
<200800613>
1. It is still not safe of last change for build_sco_frag. make more safe way to judge. See
	Frag_buildFrag(sco part) for detail.
------------------------------------------------------------------------
<200800613>
1. Sometime, reading register does not work, that is, the read operation is successful,but
	we can't get the value of register. What we change in this version is making patch for
	this case. Pls refer VendorCmdActivateByte() for detail.
------------------------------------------------------------------------
<200800616>
1. Increase delay time to wait PCI ready after retore from S3
------------------------------------------------------------------------
<200800618>
1. Discard data packet from keyboard or mouse when connection is not competed, because
	these packets can make driver's buffer overflow
2. Transport driver to 64bit OS
3. Reset more structure in TASK_RESET(). Fix bugs of IVT's unexpectable quit
4. Patch for vista. Don't submit irp in vista when IRP returns failure
------------------------------------------------------------------------
<200800619_1>
1. Fix errors when compling in XP system which introduced in last version
------------------------------------------------------------------------
<200800620>
1. A bug was introduced to bt driver in version "btusb080618" when fix bugs of IVT'S
	unexpected quit.  There is an operation of accessing hardware in DISPATCH_LEVEL.
------------------------------------------------------------------------
<200800623>
1. Fix vista standby bug, do the D3 operation when receiving info from WB 
------------------------------------------------------------------------
<200800624>
1. Patch for  vista hibernation bug. when received info from wb and In combo state, disconnect 
  	all the connections and delay a while; In bt only state, do the same things as D3,then
  	delay a while.
 ------------------------------------------------------------------------
<200800627>
1. Patch for vista of irp fail bug. If irp completion failed on vista system, we can't resubmit
	immediately, otherwise the operation will fail.  Now, we create a task to do this, reset
	pipe firstly, and then resubmit the irp.    This can make more robustly on vista.
 ------------------------------------------------------------------------
<200800630>
1. judge device state in watchdog timer. when unplug card when restoring from D3, driver
	may start watchdog timer unintently and will block "IRP_REMOVE",thus make driver
	can't be unloaded.
 ------------------------------------------------------------------------
<200800702>
1. Fix value 'maxpower' from EEPROM to 60, according to newest DSP code
 ------------------------------------------------------------------------
<200800703>
1. Don't resubmit irp after return failure when hibernation or combo switch
 ------------------------------------------------------------------------
<200800704>
1. Patch for vista hibernation bug. Enter D3 mode when receiving System Set Power Irp
 ------------------------------------------------------------------------
<200800708>
1. Verify the process of download 8051 FW, this can fix the bug of hibernation in vista
2. check return status when accessing hareware and process according different return status
3. check sco pointer when use, to avoid BSD on multi-process system when repeately connect-disconnect
	sco device.
 ------------------------------------------------------------------------
<200800709>
1. add test mode support
2. Add some delay time when wlan requet 8051 goto mainloop in combo mode. otherwise, bt driver
	will quit for 8051 busy.
 ------------------------------------------------------------------------
<200800710>
1. Fix some bugs of test mode
 ------------------------------------------------------------------------
<200800716>
1. verify the backup registers and bd addr when combo switch or hibernation, to make sure we write
	the critical registers correctly
 ------------------------------------------------------------------------
<200800717>
1. change INF file's error when installing error on XP  system.
2. Fix bug of combo swith. Should report "Not Ok" to WB until btpower_powerd0() exit
 ------------------------------------------------------------------------
<200800723>
1. Fix the process of parsing crc error packet.
2. Change some testmode code when writing command indicator,now we don't create workitem to do
	this.
3. Fix bugs of connection failure in win2000, just ignore the error "STATUS_DEVICE_DATA_ERROR"
	when query DMA space
4. Fix a bug of BSD when suprised removing card in win2000 OS, we should wait until all the cancelled
	irp returned.
 ------------------------------------------------------------------------
<200800724>
1. stop protect inquiry timer when necessary, prevent reporting repeated inquiry complete events
2. change some code for testmode. make sure the lmp pdu has been sent when write cmd indicator
3. patch for win2000 OS's control pipe failure 
 ------------------------------------------------------------------------
<200800728>
1. Add error handler when start dsp failure, we should synchronous the cancelled irp instead of sleep,
	sleep is not safe method to wait irp returned.
2. don't check tx queue when query dma failure,try to query again
 ------------------------------------------------------------------------
<200800801>
1. Reject IVT's command when in combo switch state
2. Change cmd length of "query dma space" from 1 byte to 4 bytes. 1byte can cause control pipe
	stall frequently, which lead to a lower thoughput.
3. Roll back the patch for win2000, because we fixed the control pipe stall bug when query dma space.
4. wiite edr mode register with RegApi interface.
 ------------------------------------------------------------------------
<200800805>
1. verify the page and connection para registers after write them to DSP, make sure the registers are
	written correctly.
2. add two files to projects: "bt_flush.c" and "bt_flush.h", but not work now
3. add one more status, WB can poll this status for more info about driver status
4. add ioctl to support WB to get local bd addr
 ------------------------------------------------------------------------
<200800806>
1. add global spinlock in interrupt routine
2. check cmd status when changing into "cmd status idle"
3. add code for flush function
 ------------------------------------------------------------------------
<200800811>
1. Adjust defined values of driver status, for co-working with WB
 ------------------------------------------------------------------------
<200800818>
1. change for DTM test.  don't del device from system when receiving "PNP STOP" cmd
 ------------------------------------------------------------------------
<200800819>
1. fix a bug of dead lock, delete lock in interrupt handler
2. check driver status when read usb register in loop
3. do not retry when query dma space cmd failure
 ------------------------------------------------------------------------
<200800820>
1. fix a bug of handling "pnp stop" cmd, for dtm test
 ------------------------------------------------------------------------
<200800821>
1. reset "driver_halt_flag" in powerd0 routine, give a chance to access hw
2. rewrite "Hci_ReleaseAllConnection()" and "Hci_SuprisedReleaseAllConnection", prevent
	the contetion of operating device link. this is caused by WB: WB will stop ivt service and
	there are two thread operating the device link meanwhile
3. Fix warnings of INF file, for DTM test
------------------------------------------------------------------------
*/
#ifndef __BT_VERSION_H_
#define __BT_VERSION_H_

#define MAXVERSIONLENTH     64

// This string is displayed in the about box of the WB Utility
#define BT_STRING_VERSION_ID "BTU101209"


#endif /* _VERSION_H_ */
