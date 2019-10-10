-- Copyright 2018-2019 Delft University of Technology
--
-- Licensed under the Apache License, Version 2.0 (the "License");
-- you may not use this file except in compliance with the License.
-- You may obtain a copy of the License at
--
--     http://www.apache.org/licenses/LICENSE-2.0
--
-- Unless required by applicable law or agreed to in writing, software
-- distributed under the License is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
-- See the License for the specific language governing permissions and
-- limitations under the License.

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

package Profile_pkg is

  component ProfilerStreams is
    generic (
      OUT_COUNT_MAX   : integer := 1023;
      OUT_COUNT_WIDTH : integer := 10
    );
    port (
      pcd_clk     : in  std_logic;
      pcd_reset   : in  std_logic;
      probe_valid : in  std_logic;
      probe_ready : out std_logic;
      enable      : in  std_logic := '1';
      ecount      : out std_logic_vector(OUT_COUNT_WIDTH-1 downto 0);
      vcount      : out std_logic_vector(OUT_COUNT_WIDTH-1 downto 0);
      rcount      : out std_logic_vector(OUT_COUNT_WIDTH-1 downto 0);
      tcount      : out std_logic_vector(OUT_COUNT_WIDTH-1 downto 0);
      pcount      : out std_logic_vector(OUT_COUNT_WIDTH-1 downto 0)
    );
  end component;
  
end Profile_pkg;
