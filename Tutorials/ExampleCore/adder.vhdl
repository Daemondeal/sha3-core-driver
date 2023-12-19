library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity adder is
  port (
    i_Operand0: in std_logic_vector(31 downto 0);
    i_Operand1: in std_logic_vector(31 downto 0);
    o_Result:  out std_logic_vector(31 downto 0)
  );
end adder;

architecture Behav of adder is
begin
  o_Result <= std_logic_vector(unsigned(i_Operand0) + unsigned(i_Operand1));
end Behav;