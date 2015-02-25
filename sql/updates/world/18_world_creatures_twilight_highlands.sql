-- Twilight Shadeprowler
SET @ENTRY := 46526;
SET @SOURCETYPE := 0;

DELETE FROM `smart_scripts` WHERE `entryorguid`=@ENTRY AND `source_type`=@SOURCETYPE;
UPDATE creature_template SET AIName="SmartAI" WHERE entry=@ENTRY LIMIT 1;
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES 
(@ENTRY,@SOURCETYPE,0,0,1,0,100,0,1000,1000,5000,5000,49,0,0,0,0,0,0,18,50,0,0,0.0,0.0,0.0,0.0,"OOC - Attack Nearest Player"),
(@ENTRY,@SOURCETYPE,1,0,0,0,100,0,2000,4000,5000,6500,11,14873,0,0,0,0,0,2,0,0,0,0.0,0.0,0.0,0.0,"IC - Cast Sinister Strike");

DELETE FROM `creature` WHERE `guid`=757787;
DELETE FROM `creature_addon` WHERE `guid`=757787;
UPDATE `creature_template` SET `unit_flags`=768 WHERE `entry`=46607;