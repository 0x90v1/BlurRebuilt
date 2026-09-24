
Usage:

InjectSorceryData

	-sorcery <directory>		Where to find Sorcery data
	
	-database <name>			Name of the database to create the "_strings" table in
	
	-values <vals>				List of space separated pairs of "RecordType:FieldName" pairs to upload
	
	
Example usage:

You need a server.ini file in the working folder for the command. An example format follows at the end of this file

InjectSorceryData -sorcery z:\amax\sorcery3\main -database amax -values "AmaxVehicle:Name Route:Name AmaxEvent:Name RaceBookVehicle:Name"


An example server.ini is:

ip=uklplslave02
username=datalog
password=datalog
