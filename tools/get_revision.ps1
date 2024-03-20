$revision = 0

$answer = svnversion
$has_svn = $?
#$answer=$answer.Trim("MSP:")
$answer=$answer.substring(0,5);
$is_number = ($null -ne ($answer -as [int]))
if($has_svn -and $is_number)
{
	# svnversion valid number
	$revision = [int]$answer
}
else
{
	# try git
	$answer = git log -1
	if($?) {
		# we have git!
		$answer = $answer | Select-String -Pattern 'git-svn-id:'
		if($null -ne $answer) {
			$answer = $answer -match "trunk\@(.*) "
			$answer = $matches[1]
			$is_number = ($null -ne ($answer -as [int]))
		}
		if(!$is_number) {
			# not a standard git use some random number ...
			$answer=git rev-list --count --first-parent HEAD
			$is_number = ($null -ne ($answer -as [int]))
			if($is_number) {
				# svnversion valid number
				$answer = [int]$answer
				$answer = $answer + 331
			}
		}
	}	
}

if(!$is_number -and $has_svn) {
	# svn cammnd exists but not repo
	$answer = svn info svn://servers.simutrans.org/simutrans
	$answer = $answer | Select-String -Pattern 'Revision:'
	$a=$answer.tostring();
	$answer = $a.Substring(10)
	$is_number = ($null -ne ($answer -as [int]))
}

if($is_number) {
	# valid revision ...
	$answer
	if($args.count -eq 1) {
		# create revision.h
		"Create $($args[0])"
		$revfile = "#define REVISION $answer`n"
		$revfile | Out-File $args[0]
	}
}
else {
	"0"
	if($args.count -eq 1) {
		# create revision.h
		$revfile = "#define REVISION unrevisioned`n"
		$revfile | Out-File $($args[0])
	}
}	
