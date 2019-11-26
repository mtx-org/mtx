<?php
  include('dbms_inc.php');

  $link = mysql_connect($mysql_host,$mysql_user,$mysql_password)
	or die("Could not connect");

  mysql_select_db($mysql_dbms) or die("Could not select database");

  $record = $_GET['record'];
  
  $query_str="select * from loaders where id = $record";
  $result=mysql_query($query_str,$link) or die("Invalid query '$query_str'");
  $num_rows=mysql_num_rows($result);
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
  "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
  <head>
    <meta http-equiv="Content-type" content="text/html;charset=UTF-8" />
    <meta http-equiv="Pragma" content="no-cache" />

    <title>MTX Compatibility List - Details</title>

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
  </head>

  <body>
    <table style="width:100%" cellspacing="0" cellpadding="10">
      <tr valign="middle" style="background-color:Aqua">
        <th></th>
        <th style="text-align: center">
          <h1>MTX Compatibility List<br />Details</h1>
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
          The database is currently very incomplete. Please <a href="contrib.php">submit</a> new 
          entries so that others can benefit.
          <p />
          <table style="text-align:center" border="1">
            <tr>
              <th style="text-align: center" colspan="4">OS and General Info</th>
            </tr>
<?php 
  if ($num_rows == 1 && ($row = mysql_fetch_assoc($result))) {
    extract($row);
?>
            <tr>
              <th>Operating System</th>
              <td>
                <?php echo "$osname" ?>
              </td>
              <th>MTX Version</th>
              <td>
                <?php echo "$mtxversion" ?><br/>
              </td>
            </tr>
            <tr>
              <th>OS Version</th>
              <td colspan="3">
                <?php echo "$osversion" ?><br/>
              </td>
            </tr>
            <tr>
              <th>Loader Description</th>
              <td colspan="3">
                <?php echo "$description" ?><br/>
              </td>
            </tr>
            <tr>
              <th style="text-align: center" colspan="4">LoaderInfo Output</th>
            </tr>
            <tr>
              <th>Vendor ID</th>
              <td>
                <?php echo "$vendorid" ?><br/>
              </td>
              <th>Product ID</th>
              <td>
                <?php echo "$productid" ?><br/>
              </td>
            </tr>
            <tr>
              <th>Revision</th>
              <td>
                <?php echo "$revision" ?><br/>
              </td>
              <th>Serial Number</th>
              <td>
                <?php echo "$serialnum" ?><br/>
              </td>
            </tr>
            <tr>
              <th>Barcode Reader</th>
              <td>
                <?php echo $barcodes == 1 ? "Yes" : "No" ?>
              </td>
              <th>Element Address Assignment Page</th>
              <td>
                <?php echo $eaap == 1 ? "Yes" : "No" ?>
              </td>
            </tr>
            <tr>
              <th>Transport Geometry Descriptor Page</th>
              <td>
                <?php echo $tgdp == 1 ? "Yes" : "No" ?>
              </td>
              <th>Can Transfer</th>
              <td>
                <?php echo $canxfer == 1 ? "Yes" : "No" ?>
              </td>
            </tr>
            <tr>
              <th>Number of Medium Transport Elements</th>
              <td>
                <?php echo "$transports" ?>
              </td>
              <th>Number of Storage Elements</th>
              <td>
                <?php echo "$slots" ?>
              </td>
            </tr>
            <tr>
              <th>Number of Import/Export Elements</th>
              <td>
                <?php echo "$imports" ?>
              </td>
              <th>Number of Data Transfer Elements</th>
              <td>
                <?php echo "$transfers" ?>
              </td>
            </tr>
            <tr>
              <th style="text-align: center" colspan="4">Comments</th>
            </tr>
            <tr>
              <td colspan="4" style="text-align: left">
                <?php echo "$comments" ?><br/>
              </td>
            </tr>
            <tr>
              <td colspan="4">
                <table border="1" style="width:100%">
                  <tr>
                    <th style="width:50%">Submitted By</th>
                    <td style="width:50%">
                      <?php echo "$name" ?><br/>
                    </td>
                  </tr>
                </table>
              </td>
            </tr>
<?php
  } else {
?>
            <tr>
              <td style="text-align:center" colspan="5">RECORD NOT FOUND IN DATABASE</td>
            </tr>
<?php
  }
?>
		  </table>
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
