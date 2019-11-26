<?php
  include('dbms_inc.php');
  require_once 'securimage.php';

  $link = mysql_connect($mysql_host,$mysql_user,$mysql_password)
	or die("Could not connect");

  mysql_select_db($mysql_dbms) or die("Could not select database");

  $image = new Securimage();

  $image_verified = false;

  if ($image->check($_POST['l_code'])) {
    $image_verified = true;   
  }

  /* Okay, see if they submitted anything: */
  if ($_POST['l_verified'] != "" && $image_verified) {
    $image->clear();
    /* create a MySQL insert statement: */
    $ld_enabled=(int)$_POST['l_enabled'];
    $ld_worked=(int)$_POST['l_worked'];
    $ld_mtxversion=mysql_escape_string($_POST['l_mtxversion']);
    $ld_osname=mysql_escape_string($_POST['l_osname']);
    $ld_osversion=mysql_escape_string($_POST['l_osversion']);
    $ld_description=mysql_escape_string($_POST['l_description']);
    $ld_vendorid=mysql_escape_string($_POST['l_vendorid']);
    $ld_productid=mysql_escape_string($_POST['l_productid']);
    $ld_revision=mysql_escape_string($_POST['l_revision']);
    $ld_barcodes=mysql_escape_string($_POST['l_barcodes']);
    $ld_eaap=mysql_escape_string($_POST['l_eaap']);
    $ld_transports=(int)$_POST['l_transports'];
    $ld_slots=(int)$_POST['l_slots'];
    $ld_transfers=(int)$_POST['l_transfers'];
    $ld_imports=(int)$_POST['l_imports'];
    $ld_tgdp=mysql_escape_string($_POST['l_tgdp']);
    $ld_canxfer=mysql_escape_string($_POST['l_canxfer']);
    $ld_serialnum=mysql_escape_string($_POST['l_serialnum']);
    $ld_email=mysql_escape_string($_POST['l_email']);
    $ld_name=mysql_escape_string($_POST['l_name']);
    $ld_comments=mysql_escape_string($_POST['l_comments']);
    /* 'contributed' is calculated by 'now()' function */

    $query_str="insert into loaders (enabled,worked,osname,osversion,mtxversion,description,vendorid,productid,revision,barcodes,eaap,transports,slots,imports,transfers,tgdp,canxfer,serialnum,email,name,contributed,comments) values ( 1, 1, '$ld_osname', '$ld_osversion', '$ld_mtxversion', '$ld_description', '$ld_vendorid', '$ld_productid', '$ld_revision', $ld_barcodes, $ld_eaap, $ld_transports, $ld_slots, $ld_imports, $ld_transfers, $ld_tgdp, $ld_canxfer, '$ld_serialnum', '$ld_email', '$ld_name', now(), '$ld_comments') ";

    /* now to insert it */

    $result=mysql_query($query_str,$link);

    mail("administrator@opensource-sw.net","New Addition to MTX Compatibility List",$query_str);

    header("Location: http://mtx.opensource-sw.net/compatibility.php");
    exit();
  }
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
  "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
  <head>
    <meta http-equiv="Content-type" content="text/html;charset=UTF-8" />
    <meta http-equiv="Pragma" content="no-cache" />
    <meta name="Content-script-type" content="text/javascript" />

    <title>MTX Compatibility List - Verification</title>

    <style type="text/css">
      <!--
        TH { background-color: aqua; text-align: right; }
        TR { background-color: white; }
        TD { text-align: left; }
        H2 { text-align: center; }
        BODY { background-color: white; }
        A:link { color: blue; text-decoration: underline; }
        A:active { text-decoration: underline; }
        A:hover { text-decoration: underline; }
        A:visited { color: blue; text-decoration: underline; }
      -->
    </style>

    <script language="JavaScript">
      <!--
		function OnSubmitForm()
		{
		  if (document.pressed == 'Save')
		  {
			document.loader.action = "verify.php";
		  }
		  else if (document.pressed == 'Cancel' || document.pressed == 'Try Again')
		  {
			document.loader.action = "contrib.php";
		  }
		  return true;
		}
      -->
	</script>
  </head>

  <body>
    <table style="width:100%" cellspacing="0" cellpadding="10">
      <tr valign="middle">
        <th></th>
        <th style="text-align: center">
          <h1>MTX Compatibility List<br />Verification</h1>
        </th>
      </tr>
      <tr>
        <th style="vertical-align:top; text-align:left">
          <h2>Links:</h2>
          <p>
            <a href="http://sourceforge.net/projects/mtx">SourceForge Project</a>
          </p>
          <p>
            <a href="http://mtx.opensource-sw.net">Home Page</a>
          </p>
          <p>
            <a href="http://sourceforge.net/project/showfiles.php?group_id=4626">Downloads</a>
          </p>
          <p style="font-weight:bold; text-decoration:underline">Compatibility</p>
          <p>
            <a href="compatibility.php" style="margin-left: 1em">View</a>
            <br />
            <a href="contrib.php" style="margin-left: 1em">Submit</a>
            <br />
          </p>
          <p>
            <a href="faq.html">FAQ</a>
          </p>
        </th>
        <td rowspan="2">
<?php
  if (!$image_verified) {
?>
          <h2>The code you entered didn't match the image, click the Try Again button to return and retry</h2>
<?php
  } else {
?>
          You entered the following values.  If they are correct, click on "Save", 
          otherwise click on "Cancel"
<?php
  }
?>
          <!-- <p style="text-align:center"> -->
		  <form name="loader" action="verify.php" method="post" onSubmit="return OnSubmitForm();">
			<input type="hidden" name="l_verified" value="1" />
			<input type="hidden" name="l_enabled" value="1" />
			<input type="hidden" name="l_worked" value="<?php echo $_POST['l_worked'] ?>" />
			<input type="hidden" name="l_osname" value="<?php echo $_POST['l_osname'] ?>" />
			<input type="hidden" name="l_osversion" value="<?php echo $_POST['l_osversion'] ?>" />
			<input type="hidden" name="l_mtxversion" value="<?php echo $_POST['l_mtxversion'] ?>" />
			<input type="hidden" name="l_description" value="<?php echo $_POST['l_description'] ?>" />
			<input type="hidden" name="l_vendorid" value="<?php echo $_POST['l_vendorid'] ?>" />
			<input type="hidden" name="l_productid" value="<?php echo $_POST['l_productid'] ?>" />
			<input type="hidden" name="l_revision" value="<?php echo $_POST['l_revision'] ?>" />
			<input type="hidden" name="l_barcodes" value="<?php echo $_POST['l_barcodes'] ?>" />
			<input type="hidden" name="l_eaap" value="<?php echo $_POST['l_eaap'] ?>" />
			<input type="hidden" name="l_transports" value="<?php echo $_POST['l_transports'] ?>" />
			<input type="hidden" name="l_slots" value="<?php echo $_POST['l_slots'] ?>" />
			<input type="hidden" name="l_transfers" value="<?php echo $_POST['l_transfers'] ?>" />
			<input type="hidden" name="l_imports" value="<?php echo $_POST['l_imports'] ?>" />
			<input type="hidden" name="l_tgdp" value="<?php echo $_POST['l_tgdp'] ?>" />
			<input type="hidden" name="l_canxfer" value="<?php echo $_POST['l_canxfer'] ?>" />
			<input type="hidden" name="l_serialnum" value="<?php echo $_POST['l_serialnum'] ?>" />
			<input type="hidden" name="l_email" value="<?php echo $_POST['l_email'] ?>" />
			<input type="hidden" name="l_name" value="<?php echo $_POST['l_name'] ?>" />
			<input type="hidden" name="l_comments" value="<?php echo $_POST['l_comments'] ?>" />
			<input type="hidden" name="l_code" value="<?php echo $_POST['l_code'] ?>" />
<?php
  if (!$image_verified) {
?>
			<div style="text-align:center">
			  <input type="submit" name="Cancel" value="Try Again" onClick="document.pressed=this.value" />
			</div>
<?php
  } else {
?>
			<table border="1" style="text-align:left">
			  <tr>
				<th style="text-align: center" colspan="4">OS and General Info</th>
			  </tr>
			  <tr>
				<th>Operating System</th>
				<td><?php echo $_POST['l_osname'] ?></td>
				<th>MTX Version</th>
				<td><?php echo $_POST['l_mtxversion'] ?></td>
			  </tr>
			  <tr>
				<th>OS Version</th>
				<td colspan="3"><?php echo $_POST['l_osversion'] ?><br/></td>
			  </tr>
			  <tr>
				<th>Loader Description</th>
				<td colspan="3"><?php echo $_POST['l_description'] ?><br/></td>
			  </tr>
			  <tr>
				<th style="text-align: center" colspan="4">LoaderInfo Output</th>
			  </tr>
			  <tr>
				<th>Vendor ID</th>
				<td><?php echo $_POST['l_vendorid'] ?><br/></td>
				<th>Product ID</th>
				<td><?php echo $_POST['l_productid'] ?><br/></td>
			  </tr>
			  <tr>
				<th>Revision</th>
				<td><?php echo $_POST['l_revision'] ?><br/></td>
				<th>Serial Number</th>
				<td><?php echo $_POST['l_serialnum'] ?><br/></td>
			  </tr>
			  <tr>
				<th>Barcode Reader</th>
				<td><?php echo ($_POST['l_barcodes'] == 1 ? "Yes" : "No") ?></td>
				<th>Element Address Assignment Page</th>
				<td><?php echo ($_POST['l_eaap'] == 1 ? "Yes" : "No") ?></td>
			  </tr>
			  <tr>
				<th>Transport Geometry Descriptor Page</th>
				<td><?php echo ($_POST['l_tgdp'] == 1 ? "Yes" : "No") ?></td>
				<th>Can Transfer</th>
				<td><?php echo ($_POST['l_canxfer'] == 1 ? "Yes" : "No") ?></td>
			  </tr>
			  <tr>
				<th>Number of Medium Transport Elements</th>
				<td><?php echo $_POST['l_transports'] ?></td>
				<th>Number of Storage Elements</th>
				<td><?php echo $_POST['l_slots'] ?></td>
			  </tr>
			  <tr>
				<th>Number of Import/Export Elements</th>
				<td><?php echo $_POST['l_imports'] ?></td>
				<th>Number of Data Transfer Elements</th>
				<td><?php echo $_POST['l_transfers'] ?></td>
			  </tr>
			  <tr>
				<th style="text-align: center" colspan="4">Comments</th>
			  </tr>
			  <tr>
				<td colspan="4" style="text-align:center">
				<textarea name="l_comments" cols="70" rows="4"><?php echo $_POST['l_comments'] ?></textarea>
				</td>
			  </tr>
			  <tr>
				<th style="text-align: center" colspan="4">Personal Data</th>
			  </tr>
			  <tr>
				<th>Your Name</th>
				<td><?php echo $_POST['l_name'] ?><br/></td>
				<th>Your EMAIL Address </th>
				<td><?php echo $_POST['l_email'] ?><br/></td>
			  </tr>
			  <tr>
				<td colspan="2" style="text-align:right">
				  <input type="submit" name="Save" value="Save" onClick="document.pressed=this.value" />
				</td>
				<td colspan="2">
				  <input type="submit" name="Cancel" value="Cancel" onClick="document.pressed=this.value" />
				</td>
			  </tr>
			</table>
<?php
  }
?>
		  </form>
		  <br />
        </td>
      </tr>
      <tr>
        <th style="text-align:center; vertical-align:middle">
          <p>
            <a href="http://www.dreamhost.com/r.cgi?277748">
              <img src="dh-100x75.gif" alt="DreamHost.com Logo"
                   height="75" width="100" style="border:0" />
            </a>
          </p>
          <p>
            <a href="http://validator.w3.org/check?uri=referer" >
              <img src="valid-xhtml10.png" alt="Valid XHTML 1.0 Strict" 
                   height="31" width="88" style="border:0" />
            </a>
          </p>
          <p>
            <a href="http://sourceforge.net/projects/mtx">
              <img src="http://sflogo.sourceforge.net/sflogo.php?group_id=4626&amp;type=16"
                   width="150" height="40"
                   alt="Get MTX: Media Changer Tools at SourceForge.net. Fast, secure and Free Open Source software downloads" />
            </a>
          </p>
        </th>
      </tr>
	  <tr>
		<th />
		<td>
          <hr />
          <table style="font-size:small; width:100%">
            <tr>
              <td style="text-align:left; width:33%">
                Maintained by <a href="mailto:robertnelson@users.sourceforge.net">Robert Nelson</a>
              </td>
              <td style="text-align:center; width:34%">
<?php
				$ChangedDate = preg_replace('/.*: (.+) \(.*/', '\1', '$LastChangedDate: 2009-04-25 05:56:37 +0000 (Sat, 25 Apr 2009) $');
				echo "Date changed: $ChangedDate";
?>
              </td>
              <td style="text-align:right; width:33%">
<?php
				$ChangedBy = preg_replace('/.*: (.+) \$/', '\1', '$LastChangedBy: robertnelson $');
				echo "Changed by: $ChangedBy";
?>
              </td>
            </tr>
          </table>
		</td>
	  </tr>
    </table>
  </body>
</html>
