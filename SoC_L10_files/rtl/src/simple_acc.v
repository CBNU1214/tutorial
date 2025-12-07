module simple_acc (
    input wire          clk,
    input wire          rst,
    input wire [3:0]    addr,
    input wire          en,
    input wire          we,
    input wire [31:0]   din,    // Data input
    output wire [31:0]  dout
);

    // 1. Internal Registers
    // Filter Coefficients (Weights)
    reg [31:0] reg_w0, reg_w1, reg_w2;
    // Input Data Buffer (Delay Line: x[n-1], x[n-2])
    reg [31:0] reg_x0, reg_x1;
    // Output Result
    reg [31:0] reg_y;

    // 2. Control Signals (Address Decoding)
    // addr 0: Write Input Data (x[n]) & Trigger Compute
    wire write_din   = en & we & (addr == 4'd0);
    // addr 1: Read Output Result (y[n])
    wire read_dout   = en & ~we & (addr == 4'd1);
    // addr 2: Clear / Reset Internal State
    wire write_clear = en & we & (addr == 4'd2);
    // addr 3~5: Write Weights
    wire write_w0    = en & we & (addr == 4'd3);
    wire write_w1    = en & we & (addr == 4'd4);
    wire write_w2    = en & we & (addr == 4'd5);

    // 3. 1D Filter Logic (3-tap FIR)
    // Formula: y[n] = w0*x[n] + w1*x[n-1] + w2*x[n-2]
    always @(posedge clk, posedge rst) begin
        if (rst) begin
            reg_w0 <= 0; reg_w1 <= 0; reg_w2 <= 0;
            reg_x0 <= 0; reg_x1 <= 0;
            reg_y  <= 0;
        end
        else if (write_clear) begin
            reg_x0 <= 0; reg_x1 <= 0;
            reg_y  <= 0;
            // Note: Weights are usually kept unless explicitly reset
        end
        else begin
            // Weight Configuration
            if (write_w0) reg_w0 <= din;
            if (write_w1) reg_w1 <= din;
            if (write_w2) reg_w2 <= din;

            // Data Processing
            if (write_din) begin
                // Shift the delay line
                reg_x0 <= din;      // Current x[n] becomes x[n-1] for next cycle
                reg_x1 <= reg_x0;   // Current x[n-1] becomes x[n-2] for next cycle
               
                // Compute Filter Output
                // y[n] = (x[n] * w0) + (x[n-1] * w1) + (x[n-2] * w2)
                // Note: reg_x0 currently holds x[n-1], reg_x1 holds x[n-2] from previous step
                reg_y <= (din * reg_w0) + (reg_x0 * reg_w1) + (reg_x1 * reg_w2);
            end
        end
    end

    // 4. Read Data Logic
    // Using PipeReg for read timing alignment (same as original lab)
    wire load_dout_ff;
    PipeReg #(1) FF_LOAD (.CLK(clk), .RST(1'b0), .EN(en), .D(read_dout), .Q(load_dout_ff));

    assign dout = (load_dout_ff) ? reg_y : 32'd0;

endmodule
