
`timescale 1 ns / 1 ps

	module SHA3Keccak_v1_0_S00_AXI #
    (
		// Width of S_AXI data bus
		parameter integer C_S_AXI_DATA_WIDTH	= 32,
		// Width of S_AXI address bus
		parameter integer C_S_AXI_ADDR_WIDTH	= 9
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
	localparam integer CORE_ADDR_BITS = 2;
	localparam integer REG_ADDR_BITS  = 5;

	// Slave registers
	reg  [C_S_AXI_DATA_WIDTH-1:0] reg_control [3:0];
	reg  [C_S_AXI_DATA_WIDTH-1:0] reg_input   [3:0];
	wire [C_S_AXI_DATA_WIDTH-1:0] reg_status  [3:0];
	
	
	// Used to implement WSTRB signal
	wire [C_S_AXI_DATA_WIDTH-1:0]   wrdata_control [3:0];
	wire [C_S_AXI_DATA_WIDTH-1:0]   wrdata_status  [3:0];
	wire [C_S_AXI_DATA_WIDTH-1:0]   wrdata_input   [3:0];
	wire [C_S_AXI_DATA_WIDTH-1:0]   wrdata_command [3:0];
	
	// Other signals for implementing axi4 logic
	wire						 slv_reg_rden;
	wire	 					 slv_reg_wren;
	reg [C_S_AXI_DATA_WIDTH-1:0] reg_data_out;
	reg							 aw_en;
	
	// Wires used for the Keccak Core
	reg 		 sha3_reset [3:0];
	wire [31:0]  sha3_input [3:0];
	reg [1:0]    sha3_byte_to_send [3:0];
	reg          sha3_is_sending_bytes [3:0];
	reg  		 sha3_is_last [3:0];
	wire 		 sha3_buffer_full [3:0];
	wire 		 sha3_out_ready [3:0];

	wire 		 sha3_reset_512;
	wire [31:0]  sha3_input_512;
	wire [1:0]    sha3_byte_to_send_512;
	wire          sha3_is_sending_bytes_512;
	wire  		 sha3_is_last_512;
	wire 		 sha3_buffer_full_512;
	wire 		 sha3_out_ready_512;
  
	wire 		 sha3_reset_224;
	wire [31:0]  sha3_input_224;
	wire [1:0]    sha3_byte_to_send_224;
	wire          sha3_is_sending_bytes_224;
	wire  		 sha3_is_last_224;
	wire 		 sha3_buffer_full_224;
	wire 		 sha3_out_ready_224;

	// WORKAROUND TO VCD FILES NOT SUPPORTING ARRAYS
	assign sha3_reset_512 = sha3_reset [0];
	assign sha3_input_512 = sha3_input [0];
	assign sha3_byte_to_send_512 = sha3_byte_to_send [0];
	assign sha3_is_sending_bytes_512 = sha3_is_sending_bytes [0];
	assign sha3_is_last_512 = sha3_is_last [0];
	assign sha3_buffer_full_512 = sha3_buffer_full [0];
	assign sha3_out_ready_512 = sha3_out_ready [0];

	assign sha3_reset_224 = sha3_reset [3];
	assign sha3_input_224 = sha3_input [3];
	assign sha3_byte_to_send_224 = sha3_byte_to_send [3];
	assign sha3_is_sending_bytes_224 = sha3_is_sending_bytes [3];
	assign sha3_is_last_224 = sha3_is_last [3];
	assign sha3_buffer_full_224 = sha3_buffer_full [3];
	assign sha3_out_ready_224 = sha3_out_ready [3];

	wire [511:0] sha3_output_512;
	wire [383:0] sha3_output_384;
	wire [255:0] sha3_output_256;
	wire [223:0] sha3_output_224;

	wire [511:0] sha3_output [3:0];


	assign sha3_output[0] = sha3_output_512;
	assign sha3_output[1][511:511-383] = sha3_output_384;
	assign sha3_output[2][511:511-255] = sha3_output_256;
	assign sha3_output[3][511:511-223] = sha3_output_224;

	genvar j;

	generate
		for (j = 0; j < 4; j = j + 1) begin
			assign reg_status[j][0]    = sha3_out_ready   [j];
			assign reg_status[j][1]    = sha3_buffer_full [j];
			assign reg_status[j][31:2] = 0;

			assign sha3_input[j] = reg_input[j];
		end

	endgenerate
	

	keccak 
	 sha512_core (
	   .clk(S_AXI_ACLK), 
	   .reset(sha3_reset[0]),
	   .in(sha3_input[0]), 
	   .in_ready(sha3_is_sending_bytes[0]),
	   .is_last(sha3_is_last[0]), 
	   .byte_num(sha3_byte_to_send[0]), 
	   .buffer_full(sha3_buffer_full[0]), 
	   .out(sha3_output_512), 
	   .out_ready(sha3_out_ready[0])
	);

	keccak #(.OUTBITS(384), .R_BITRATE(832))
	 sha384_core (
	   .clk(S_AXI_ACLK), 
	   .reset(sha3_reset[1]),
	   .in(sha3_input[1]), 
	   .in_ready(sha3_is_sending_bytes[1]),
	   .is_last(sha3_is_last[1]), 
	   .byte_num(sha3_byte_to_send[1]), 
	   .buffer_full(sha3_buffer_full[1]), 
	   .out(sha3_output_384), 
	   .out_ready(sha3_out_ready[1])
	);

	keccak #(.OUTBITS(256), .R_BITRATE(1088))
	 sha256_core (
	   .clk(S_AXI_ACLK), 
	   .reset(sha3_reset[2]),
	   .in(sha3_input[2]), 
	   .in_ready(sha3_is_sending_bytes[2]),
	   .is_last(sha3_is_last[2]), 
	   .byte_num(sha3_byte_to_send[2]), 
	   .buffer_full(sha3_buffer_full[2]), 
	   .out(sha3_output_256), 
	   .out_ready(sha3_out_ready[2])
	);

	keccak #(.OUTBITS(224), .R_BITRATE(1152))
	 sha224_core (
	   .clk(S_AXI_ACLK), 
	   .reset(sha3_reset[3]),
	   .in(sha3_input[3]), 
	   .in_ready(sha3_is_sending_bytes[3]),
	   .is_last(sha3_is_last[3]), 
	   .byte_num(sha3_byte_to_send[3]), 
	   .buffer_full(sha3_buffer_full[3]), 
	   .out(sha3_output_224), 
	   .out_ready(sha3_out_ready[3])
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
	
	generate
		for (j = 0; j < 4; j = j + 1) begin
			assign wrdata_control[j] = apply_wstrb(reg_control[j], S_AXI_WDATA, S_AXI_WSTRB);
			assign wrdata_input[j]   = apply_wstrb(reg_input[j],   S_AXI_WDATA, S_AXI_WSTRB);
			assign wrdata_command[j] = apply_wstrb(0, S_AXI_WDATA, S_AXI_WSTRB);
		end
	endgenerate

	integer core_idx;
	integer reg_idx;
	integer i;

	
	always @( posedge S_AXI_ACLK )
    begin
        if (!S_AXI_ARESETN) begin
			for (i = 0; i < 4; i = i + 1) begin
	    		reg_control[i] <= 0;
	    		reg_input[i] <= 0;
	
	    		sha3_byte_to_send[i] <= 0;
				sha3_is_sending_bytes[i] <= 0;
				sha3_reset[i] <= 1;
				sha3_is_last[i] <= 0;
			end

        end else if (slv_reg_wren) begin
			core_idx = axi_awaddr[ADDR_LSB + REG_ADDR_BITS +: CORE_ADDR_BITS];
			reg_idx  = axi_awaddr[ADDR_LSB +: REG_ADDR_BITS];

			if (reg_idx == 5'h00) begin
				reg_control[core_idx] <= wrdata_control[core_idx];
			end else if (reg_idx == 5'h02) begin
				reg_input[core_idx] <= wrdata_input[core_idx];
                // Set appropriate bits in the peripheral 
                // according to the control register.
                // Control Register:
                // Bit 1:0 - Amount of bits to transfer
                // Bit 2   - Is this the last transmission?

				if (reg_control[core_idx][2] == 1) begin
					sha3_is_last[core_idx] <= 1;
					sha3_byte_to_send[core_idx] <= reg_control[core_idx][1:0];
				end

				sha3_is_sending_bytes[core_idx] <= 1;
			end else if (reg_idx == 5'h03) begin
				if (wrdata_command[core_idx][0] == 1) begin
					sha3_reset[core_idx] <= 1;
				end
			end
        end else begin
			for (i = 0; i < 4; i = i + 1) begin
            	sha3_reset[i] <= 0;
            	sha3_is_sending_bytes[i] <= 0;        
				sha3_is_last[i] <= 0;
			end
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

	wire [1:0] core_idx_r;
	wire [5:0] reg_idx_r; 

	assign core_idx_r[1:0] = axi_awaddr[ADDR_LSB + REG_ADDR_BITS +: CORE_ADDR_BITS];
	assign reg_idx_r[5:0] =  axi_araddr[ADDR_LSB +: REG_ADDR_BITS];

	// Implement memory mapped register select and read logic generation
	// Slave register read enable is asserted when valid address is available
	// and the slave is ready to accept the read address.
	assign slv_reg_rden = axi_arready & S_AXI_ARVALID & ~axi_rvalid;
	always @(*)
	begin
		core_idx = axi_araddr[ADDR_LSB + REG_ADDR_BITS +: CORE_ADDR_BITS];
		reg_idx  = axi_araddr[ADDR_LSB +: REG_ADDR_BITS];

	      // Address decoding for reading registers
		case (reg_idx)
	        5'h00   : reg_data_out <= reg_control[core_idx];
	        5'h01   : reg_data_out <= reg_status[core_idx];
	        5'h02   : reg_data_out <= reg_input[core_idx];
	        5'h03   : reg_data_out <= 0; // NOTE: reg_command cannot be read 
	        
	        // Reading the massive SHA3-512 512-bit output
	        5'h04   : reg_data_out <= sha3_output[core_idx][511:480];
	        5'h05   : reg_data_out <= sha3_output[core_idx][479:448];
	        5'h06   : reg_data_out <= sha3_output[core_idx][447:418];
	        5'h07   : reg_data_out <= sha3_output[core_idx][417:384];
	        5'h08   : reg_data_out <= sha3_output[core_idx][383:352];
	        5'h09   : reg_data_out <= sha3_output[core_idx][351:320];
	        5'h0A   : reg_data_out <= sha3_output[core_idx][319:288];
	        5'h0B   : reg_data_out <= sha3_output[core_idx][287:256];
	        5'h0C   : reg_data_out <= sha3_output[core_idx][255:224];
	        5'h0D   : reg_data_out <= sha3_output[core_idx][223:192];
	        5'h0E   : reg_data_out <= sha3_output[core_idx][191:160];
	        5'h0F   : reg_data_out <= sha3_output[core_idx][159:128];
	        5'h10   : reg_data_out <= sha3_output[core_idx][127:96 ];
	        5'h11   : reg_data_out <= sha3_output[core_idx][95 :64 ];
	        5'h12   : reg_data_out <= sha3_output[core_idx][63 :32 ];
	        5'h13   : reg_data_out <= sha3_output[core_idx][31 :0  ];
	        
	        default : reg_data_out <= 0;
		endcase
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
