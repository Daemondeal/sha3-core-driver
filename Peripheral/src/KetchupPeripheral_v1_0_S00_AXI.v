`timescale 1 ns / 1 ps

`define SHA_BITRATE(size) (1600 - 2 * size)

	module  KetchupPeripheral_v1_0_S00_AXI #
    (
		// Width of S_AXI data bus
		parameter integer C_S_AXI_DATA_WIDTH	= 32,
		// Width of S_AXI address bus
		parameter integer C_S_AXI_ADDR_WIDTH	= 7
	)
	(
		// Global Clock Signal
		input wire  S_AXI_ACLK,
		// Global Reset Signal. This Signal is Active LOW
		input wire  S_AXI_ARESETN,
		// Write address (issued by master, acceped by Slave)
		input wire [C_S_AXI_ADDR_WIDTH-1 : 0] S_AXI_AWADDR,
		// Write channel Protection type. This signal indicates the
    		// privilege and security level of the transaction, and whether
    		// the transaction is a data access or an instruction access.
		input wire [2 : 0] S_AXI_AWPROT,
		// Write address valid. This signal indicates that the master signaling
    		// valid write address and control information.
		input wire  S_AXI_AWVALID,
		// Write address ready. This signal indicates that the slave is ready
    		// to accept an address and associated control signals.
		output wire  S_AXI_AWREADY,
		// Write data (issued by master, acceped by Slave) 
		input wire [C_S_AXI_DATA_WIDTH-1 : 0] S_AXI_WDATA,
		// Write strobes. This signal indicates which byte lanes hold
    		// valid data. There is one write strobe bit for each eight
    		// bits of the write data bus.    
		input wire [(C_S_AXI_DATA_WIDTH/8)-1 : 0] S_AXI_WSTRB,
		// Write valid. This signal indicates that valid write
    		// data and strobes are available.
		input wire  S_AXI_WVALID,
		// Write ready. This signal indicates that the slave
    		// can accept the write data.
		output wire  S_AXI_WREADY,
		// Write response. This signal indicates the status
    		// of the write transaction.
		output wire [1 : 0] S_AXI_BRESP,
		// Write response valid. This signal indicates that the channel
    		// is signaling a valid write response.
		output wire  S_AXI_BVALID,
		// Response ready. This signal indicates that the master
    		// can accept a write response.
		input wire  S_AXI_BREADY,
		// Read address (issued by master, acceped by Slave)
		input wire [C_S_AXI_ADDR_WIDTH-1 : 0] S_AXI_ARADDR,
		// Protection type. This signal indicates the privilege
    		// and security level of the transaction, and whether the
    		// transaction is a data access or an instruction access.
		input wire [2 : 0] S_AXI_ARPROT,
		// Read address valid. This signal indicates that the channel
    		// is signaling valid read address and control information.
		input wire  S_AXI_ARVALID,
		// Read address ready. This signal indicates that the slave is
    		// ready to accept an address and associated control signals.
		output wire  S_AXI_ARREADY,
		// Read data (issued by slave)
		output wire [C_S_AXI_DATA_WIDTH-1 : 0] S_AXI_RDATA,
		// Read response. This signal indicates the status of the
    		// read transfer.
		output wire [1 : 0] S_AXI_RRESP,
		// Read valid. This signal indicates that the channel is
    		// signaling the required read data.
		output wire  S_AXI_RVALID,
		// Read ready. This signal indicates that the master can
    		// accept the read data and response information.
		input wire  S_AXI_RREADY
	);

	// AXI4LITE signals
	reg [C_S_AXI_ADDR_WIDTH-1 : 0] 	axi_awaddr;
	reg  							axi_awready;
	reg  							axi_wready;
	reg [1 : 0] 					axi_bresp;
	reg  							axi_bvalid;
	reg [C_S_AXI_ADDR_WIDTH-1 : 0] 	axi_araddr;
	reg  							axi_arready;
	reg [C_S_AXI_DATA_WIDTH-1 : 0] 	axi_rdata;
	reg [1 : 0]					 	axi_rresp;
	reg  							axi_rvalid;

	// ADDR_LSB is used for addressing 32/64 bit registers/memories
	// ADDR_LSB = 2 for 32 bits (n downto 2)
	// ADDR_LSB = 3 for 64 bits (n downto 3)
	localparam integer ADDR_LSB = (C_S_AXI_DATA_WIDTH/32) + 1;

	// 32 bits needed to address each core = 7 bits addressing
	localparam integer REG_ADDR_BITS  = 5;

	// Slave registers
	reg  [C_S_AXI_DATA_WIDTH-1:0] reg_control;
	reg  [C_S_AXI_DATA_WIDTH-1:0] reg_input;
	wire [C_S_AXI_DATA_WIDTH-1:0] reg_status;
	
	
	// Used to implement WSTRB signal
	wire [C_S_AXI_DATA_WIDTH-1:0]   wrdata_control;
	wire [C_S_AXI_DATA_WIDTH-1:0]   wrdata_status;
	wire [C_S_AXI_DATA_WIDTH-1:0]   wrdata_input;
	wire [C_S_AXI_DATA_WIDTH-1:0]   wrdata_command;
	
	// Other signals for implementing axi4 logic
	wire						 slv_reg_rden;
	wire	 					 slv_reg_wren;
	reg [C_S_AXI_DATA_WIDTH-1:0] reg_data_out;
	reg							 aw_en;
	
	// Wires used for the Keccak Core
	reg 		 sha3_reset;
	wire [31:0]  sha3_input;
	reg [1:0]    sha3_byte_to_send;
	reg          sha3_is_sending_bytes;
	reg  		 sha3_is_last;
	wire 		 sha3_buffer_full;
	wire 		 sha3_out_ready;
	wire [1:0]   sha3_out_size;

	wire [511:0] sha3_output;
	wire [511:0] sha3_core_output;

	assign sha3_output = sha3_core_output;

	assign sha3_out_size[1:0] = reg_control[5:4];

	assign reg_status[0]    = sha3_out_ready;
	assign reg_status[1]    = sha3_buffer_full;
	assign reg_status[31:2] = 0;

	assign sha3_input = reg_input;

	

	keccak 
	 sha512_core (
	   .clk(S_AXI_ACLK), 
	   .reset(sha3_reset),
	   .in(sha3_input), 
	   .in_ready(sha3_is_sending_bytes),
	   .is_last(sha3_is_last), 
	   .byte_num(sha3_byte_to_send), 
	   .buffer_full(sha3_buffer_full), 
	   .out(sha3_core_output), 
	   .out_ready(sha3_out_ready),
	   .out_size(sha3_out_size)
	);


	// I/O Connections assignments
	assign S_AXI_AWREADY = axi_awready;
	assign S_AXI_WREADY	 = axi_wready;
	assign S_AXI_BRESP	 = axi_bresp;
	assign S_AXI_BVALID  = axi_bvalid;
	assign S_AXI_ARREADY = axi_arready;
	assign S_AXI_RDATA	 = axi_rdata;
	assign S_AXI_RRESP	 = axi_rresp;
	assign S_AXI_RVALID	 = axi_rvalid;

	// Implement axi_awready generation
	// axi_awready is asserted for one S_AXI_ACLK clock cycle when both
	// S_AXI_AWVALID and S_AXI_WVALID are asserted. axi_awready is
	// de-asserted when reset is low.
	always @( posedge S_AXI_ACLK )
	begin
	  if ( S_AXI_ARESETN == 1'b0 )
	    begin
	      axi_awready <= 1'b0;
	      aw_en <= 1'b1;
	    end 
	  else
	    begin    
	      if (~axi_awready && S_AXI_AWVALID && S_AXI_WVALID && aw_en)
	        begin
	          // slave is ready to accept write address when 
	          // there is a valid write address and write data
	          // on the write address and data bus. This design 
	          // expects no outstanding transactions. 
	          axi_awready <= 1'b1;
	          aw_en <= 1'b0;
	        end
	        else if (S_AXI_BREADY && axi_bvalid)
	            begin
	              aw_en <= 1'b1;
	              axi_awready <= 1'b0;
	            end
	      else           
	        begin
	          axi_awready <= 1'b0;
	        end
	    end 
	end       

	// Implement axi_awaddr latching
	// This process is used to latch the address when both 
	// S_AXI_AWVALID and S_AXI_WVALID are valid. 
	always @( posedge S_AXI_ACLK )
	begin
	  if ( S_AXI_ARESETN == 1'b0 )
	    begin
	      axi_awaddr <= 0;
	    end 
	  else
	    begin    
	      if (~axi_awready && S_AXI_AWVALID && S_AXI_WVALID && aw_en)
	        begin
	          // Write Address latching 
	          axi_awaddr <= S_AXI_AWADDR;
	        end
	    end 
	end       

	// Implement axi_wready generation
	// axi_wready is asserted for one S_AXI_ACLK clock cycle when both
	// S_AXI_AWVALID and S_AXI_WVALID are asserted. axi_wready is 
	// de-asserted when reset is low. 
	always @( posedge S_AXI_ACLK )
	begin
	  if ( S_AXI_ARESETN == 1'b0 )
	    begin
	      axi_wready <= 1'b0;
	    end 
	  else
	    begin    
	      if (~axi_wready && S_AXI_WVALID && S_AXI_AWVALID && aw_en )
	        begin
	          // slave is ready to accept write data when 
	          // there is a valid write address and write data
	          // on the write address and data bus. This design 
	          // expects no outstanding transactions. 
	          axi_wready <= 1'b1;
	        end
	      else
	        begin
	          axi_wready <= 1'b0;
	        end
	    end 
	end       

	// Used to implement the WSTRB signal. 
	// You need to only pass a new byte if the corresponding
	// bit in the write strobe is 1, otherwise you should just
	// keep the old  value.
	function [C_S_AXI_DATA_WIDTH-1:0] apply_wstrb;
	   input [C_S_AXI_DATA_WIDTH-1:0]    prior_data;
	   input [C_S_AXI_DATA_WIDTH-1:0]    new_data;
	   input [C_S_AXI_DATA_WIDTH/8-1:0]  write_strobe;
	   
	   integer k;
	   for (k = 0; k < C_S_AXI_DATA_WIDTH/8; k = k + 1) begin
	       apply_wstrb[k * 8 +: 8]
	           = write_strobe[k] ? new_data[k * 8 +: 8] : prior_data[k * 8 +: 8];	   
	   end
	endfunction

	// Implement memory mapped register select and write logic generation
	// The write data is accepted and written to memory mapped registers when
	// axi_awready, S_AXI_WVALID, axi_wready and S_AXI_WVALID are asserted. Write strobes are used to
	// select byte enables of slave registers while writing.
	// These registers are cleared when reset (active low) is applied.
	// Slave register write enable is asserted when valid address and data are available
	// and the slave is ready to accept the write address and write data.
	assign slv_reg_wren = axi_wready && S_AXI_WVALID && axi_awready && S_AXI_AWVALID;
	
	assign wrdata_control = apply_wstrb(reg_control, S_AXI_WDATA, S_AXI_WSTRB);
	assign wrdata_input   = apply_wstrb(reg_input,   S_AXI_WDATA, S_AXI_WSTRB);
	assign wrdata_command = apply_wstrb(0,           S_AXI_WDATA, S_AXI_WSTRB);

	integer reg_idx;

	
	always @( posedge S_AXI_ACLK )
    begin
        if (!S_AXI_ARESETN) begin
	    		reg_control <= 0;
	    		reg_input <= 0;
	
	    		sha3_byte_to_send <= 0;
				sha3_is_sending_bytes <= 0;
				sha3_reset <= 1;
				sha3_is_last <= 0;

        end else if (slv_reg_wren) begin
			reg_idx  = axi_awaddr[ADDR_LSB +: REG_ADDR_BITS];

			if (reg_idx == 5'h00) begin
				reg_control <= wrdata_control;
			end else if (reg_idx == 5'h02) begin
				reg_input <= wrdata_input;
                // Set appropriate bits in the peripheral 
                // according to the control register.
                // Control Register:
                // Bit 1:0 - Amount of bits to transfer
                // Bit 2   - Is this the last transmission?
				// Bit 3   - Reserved
				// Bit 5:4 - Size of output

				if (reg_control[2] == 1) begin
					sha3_is_last <= 1;
					sha3_byte_to_send <= reg_control[1:0];
				end

				sha3_is_sending_bytes <= 1;
			end else if (reg_idx == 5'h03) begin
				if (wrdata_command[0] == 1) begin
					sha3_reset <= 1;
				end
			end
        end else begin
            sha3_reset <= 0;
            sha3_is_sending_bytes <= 0;        
			sha3_is_last <= 0;
        end
    end

	// Implement write response logic generation
	// The write response and response valid signals are asserted by the slave 
	// when axi_wready, S_AXI_WVALID, axi_wready and S_AXI_WVALID are asserted.  
	// This marks the acceptance of address and indicates the status of 
	// write transaction.
	always @( posedge S_AXI_ACLK )
	begin
	  if ( S_AXI_ARESETN == 1'b0 )
	    begin
	      axi_bvalid  <= 0;
	      axi_bresp   <= 2'b0;
	    end 
	  else
	    begin    
	      if (axi_awready && S_AXI_AWVALID && ~axi_bvalid && axi_wready && S_AXI_WVALID)
	        begin
	          // indicates a valid write response is available
	          axi_bvalid <= 1'b1;
	          axi_bresp  <= 2'b0; // 'OKAY' response 
	        end                   // work error responses in future
	      else
	        begin
	          if (S_AXI_BREADY && axi_bvalid) 
	            //check if bready is asserted while bvalid is high) 
	            //(there is a possibility that bready is always asserted high)   
	            begin
	              axi_bvalid <= 1'b0; 
	            end  
	        end
	    end
	end   

	// Implement axi_arready generation
	// axi_arready is asserted for one S_AXI_ACLK clock cycle when
	// S_AXI_ARVALID is asserted. axi_awready is 
	// de-asserted when reset (active low) is asserted. 
	// The read address is also latched when S_AXI_ARVALID is 
	// asserted. axi_araddr is reset to zero on reset assertion.
	always @( posedge S_AXI_ACLK )
	begin
	  if ( S_AXI_ARESETN == 1'b0 )
	    begin
	      axi_arready <= 1'b0;
	      axi_araddr  <= 32'b0;
	    end 
	  else
	    begin    
	      if (~axi_arready && S_AXI_ARVALID)
	        begin
	          // indicates that the slave has acceped the valid read address
	          axi_arready <= 1'b1;
	          // Read address latching
	          axi_araddr  <= S_AXI_ARADDR;
	        end
	      else
	        begin
	          axi_arready <= 1'b0;
	        end
	    end 
	end       

	// Implement axi_arvalid generation
	// axi_rvalid is asserted for one S_AXI_ACLK clock cycle when both 
	// S_AXI_ARVALID and axi_arready are asserted. The slave registers 
	// data are available on the axi_rdata bus at this instance. The 
	// assertion of axi_rvalid marks the validity of read data on the 
	// bus and axi_rresp indicates the status of read transaction.axi_rvalid 
	// is deasserted on reset (active low). axi_rresp and axi_rdata are 
	// cleared to zero on reset (active low).  
	always @( posedge S_AXI_ACLK )
	begin
	  if ( S_AXI_ARESETN == 1'b0 )
	    begin
	      axi_rvalid <= 0;
	      axi_rresp  <= 0;
	    end 
	  else
	    begin    
	      if (axi_arready && S_AXI_ARVALID && ~axi_rvalid)
	        begin
	          // Valid read data is available at the read data bus
	          axi_rvalid <= 1'b1;
	          axi_rresp  <= 2'b0; // 'OKAY' response
	        end   
	      else if (axi_rvalid && S_AXI_RREADY)
	        begin
	          // Read data is accepted by the master
	          axi_rvalid <= 1'b0;
	        end                
	    end
	end    

	// Implement memory mapped register select and read logic generation
	// Slave register read enable is asserted when valid address is available
	// and the slave is ready to accept the read address.
	assign slv_reg_rden = axi_arready & S_AXI_ARVALID & ~axi_rvalid;
	integer output_idx;
	always @(*)
	begin
		reg_idx  = axi_araddr[ADDR_LSB +: REG_ADDR_BITS];

	      // Address decoding for reading registers
		case (reg_idx)
	        5'h00   : reg_data_out <= reg_control;
	        5'h01   : reg_data_out <= reg_status;
	        5'h02   : reg_data_out <= reg_input;
	        5'h03   : reg_data_out <= 0; // NOTE: reg_command cannot be read 
			default : reg_data_out <= 0;
		endcase

		// Reading an output register
		if (reg_idx <= 5'h13 && reg_idx >= 5'h04) begin
			output_idx = 15 - (reg_idx - 4);
			reg_data_out <= sha3_output[output_idx * 32 +: 32];
		end
	end

	// Output register or memory read data
	always @( posedge S_AXI_ACLK )
	begin
	  if ( S_AXI_ARESETN == 1'b0 )
	    begin
	      axi_rdata  <= 0;
	    end 
	  else
	    begin    
	      // When there is a valid read address (S_AXI_ARVALID) with 
	      // acceptance of read address by the slave (axi_arready), 
	      // output the read dada 
	      if (slv_reg_rden)
	        begin
	          axi_rdata <= reg_data_out;     // register read data
	        end   
	    end
	end    

	endmodule
